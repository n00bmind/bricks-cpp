#pragma once

struct ReflectResult
{
    enum Code : u32
    {
        Ok = 0,
        UnknownError,
        BadData,
        BufferOverflow,
    } code;
    //char const* msg;

    explicit operator bool() const { return code == Ok; }
};
const ReflectResult ReflectOk = { ReflectResult::Ok }; //, "" };

enum FieldFlags : u32
{
    None = 0,
};

struct FieldAttributes
{
    char const* description;
    struct
    {
        float min;
        float max;
    } range;
    u32 flags;
    bool readOnly;

    constexpr FieldAttributes()
        : description( nullptr )
        , range { -FLT_MAX, FLT_MAX }
        , flags( 0 )
        , readOnly( false )
    {}

    constexpr FieldAttributes& Description( char const* str )
    {
        description = str;
        return *this;
    }
    constexpr FieldAttributes& Range( float min, float max )
    {
        range = { min, max };
        return *this;
    }
    constexpr FieldAttributes& ReadOnly()
    {
        readOnly = true;
        return *this;
    }
    constexpr FieldAttributes& Flags( u32 flags_ )
    {
        flags = flags_;
        return *this;
    }
};

#define ATTRS FieldAttributes()


template <bool RW>
struct Reflector
{
    static constexpr bool IsReading =  RW;
    static constexpr bool IsWriting = !RW;

    FieldAttributes attribs;
    Allocator* allocator;
    ReflectResult error;

    bool FieldHasFlags( u32 flags ) const { return (attribs.flags & flags) == flags; }

    bool HasError() const { return error.code != ReflectResult::Ok; }
    void SetError( ReflectResult r ) { if( !HasError() ) error = r; }
    void SetError( ReflectResult::Code e ) { if( !HasError() ) error = { e }; }

protected:
    Reflector( Allocator* allocator_ )
        : attribs{}
        , allocator( allocator_ )
        , error( ReflectOk )
    {}
};


template <typename R>
struct ReflectedTypeInfo
{
    R* reflector;

    ReflectedTypeInfo( R* r )
        : reflector( r )
    {
    }
};


// Nasty trick to allow using anything as template param when R is not defined
#define BEGIN_FIELDS \
    ReflectedTypeInfo<R> _reflectInfo( &r );

// FIXME Setting attributes on the reflector like this means they're not correctly set/unset across a hierarchy
// We'd need a stack of them. Probably push them in Start and pop them in End, along with any other stuff (json stack)
template <typename R, typename F>
INLINE ReflectResult ReflectFieldBody( R& r, ReflectedTypeInfo<R>& info, u32 fieldId, F& f, StaticString const& name, FieldAttributes const& attribs )
{                                                         
    const sz fieldOffset = ReflectFieldOffset( r );      
    if( ReflectFieldStart( fieldId, name, &info, r ) ) 
    {                                                     
        ReflectResult ret = ReflectOk;

        {                                                 
            r.attribs = attribs;
            ret = Reflect( r, f );                        
            r.attribs = {};                               
        }                                                 

        r.SetError( ret );
        ReflectFieldEnd( fieldId, fieldOffset, &info, r );
    }

    return r.error;                                   
}
template <typename R, typename F>
INLINE ReflectResult ReflectFieldBody( R& r, ReflectedTypeInfo<R>& info, u32 fieldId, F& f, StaticString const& name )
{
    return ReflectFieldBody( r, info, fieldId, f, name, {} );
}
#define FIELD( id, f, ... )                 if( !ReflectFieldBody( r, _reflectInfo, id, d.f, #f,     ##__VA_ARGS__ ) ) return r.error;
#define FIELD_NAMED( id, f, name, ... )     if( !ReflectFieldBody( r, _reflectInfo, id, d.f, name,   ##__VA_ARGS__ ) ) return r.error;
#define FIELD_LOCAL( id, f, ... )           if( !ReflectFieldBody( r, _reflectInfo, id, f,   #f,     ##__VA_ARGS__ ) ) return r.error;


#define REFLECT(...)                        \
    template <typename R>                   \
    ReflectResult Reflect( R& r, __VA_ARGS__& d )

#define REFLECT_T(...)                      \
    template <typename R, typename T>       \
    ReflectResult Reflect( R& r, __VA_ARGS__& d )

// These don't work with FIELDs
#define REFLECT_SPECIAL_R(reflector, ...)   \
    ReflectResult Reflect( reflector& r, __VA_ARGS__& d )

#define REFLECT_SPECIAL_RT(reflector, ...)  \
    template <typename T>                   \
    ReflectResult Reflect( reflector& r, __VA_ARGS__& d )

#define REFLECT_SPECIAL_RW(reflector, ...)  \
    template <bool RW>                      \
    ReflectResult Reflect( reflector<RW>& r, __VA_ARGS__& d )

#define REFLECT_SPECIAL_RWT(reflector, ...) \
    template <bool RW, typename T>          \
    ReflectResult Reflect( reflector<RW>& r, __VA_ARGS__& d )


// Generic type converter for insertion in the serialization chain of any attribute
// The struct attribute to convert is the 'source' and gets passed on construction
// The actual value that gets serialized out is the 'target'. Hence:
// - A writer converts the source value to the target value, and writes the target
// - A reader reads from the target, and converts the target value to the source value

// TODO We want this to be applicable to any conversion scenarios, while at the same time be able to restrict its action
// only to specific Reflectors (or just apply to all). 
#define DEFINE_ADAPTER(name)              \
    template <typename Src, typename Tgt> \
    struct name                           \
    {                                     \
        Src& src;                         \
        Tgt tgt;                          \
                                          \
        name( Src& d ) : src( d ) , tgt{} \
        {}                                \
    };


REFLECT_T( EnumStruct<T> )
{
    T& e = *(T*)&d;

    using ValueType = T::ValueType;
    static_assert( !std::is_same< ValueType, EnumStruct<T>::InvalidValueType >(), "EnumStruct has no declared values to serialize" );

    ValueType v;
    IF( r.IsWriting )
    {
        v = e.Value();
    }

    ReflectResult ret = Reflect( r, v );
    if( !ret )
        return ret;

    IF( r.IsReading )
    {
        // TODO TEST TEST TEST !!!
        // TODO What happens if we use a struct value?
        e = T::FromValue( v );
    }

    return ReflectOk;
}

