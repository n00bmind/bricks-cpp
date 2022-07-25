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

    bool FieldHasFlags( u32 flags ) const { return (attribs.flags & flags) == flags; }

protected:
    Reflector( Allocator* allocator_ )
        : attribs{}
        , allocator( allocator_ )
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

#define ReflectFieldPush(...) true
//bool ReflectFieldPush( ... )
//{
    //return true;
//}

#define ReflectFieldPop(...)
//void ReflectFieldPop( ... )
//{

//}


// Nasty trick to allow using anything as template param when R is not defined
#define BEGIN_FIELDS \
    ReflectedTypeInfo<R> _reflectInfo( &r );

// TODO Can we remove either pair of ReflectFieldStart/End or ReflectFieldPush/Pop?
// (try to put all functionality in either of those!)
// FIXME Setting attributes on the reflector like this means they're not correctly set/unset across a hierarchy
// We'd need a stack of them. We could maybe put ReflectedTypeInfo there ?, along with any other stuff (json stack) too!
template <typename R, typename F>
INLINE ReflectResult ReflectFieldBody( R& r, ReflectedTypeInfo<R>& info, u16 id, F& f, StaticString const& name, FieldAttributes const& attribs )
{                                                         
    const int fieldOffset = ReflectFieldOffset( r );      
    if( ReflectFieldStart( id, name, &info, r ) ) 
    {                                                     
        ReflectResult ret = { ReflectResult::Ok };        
        if( ReflectFieldPush( id, name, r ) )             
        {                                                 
            r.attribs = attribs;
            ret = Reflect( r, f );                        
            r.attribs = {};                               
            ReflectFieldPop( r );                         
        }                                                 
        if( ret.code != ReflectResult::Ok )               
            return ret;                                   
                                                          
        ReflectFieldEnd( fieldOffset, r );                
    }
    return ReflectOk;
}
template <typename R, typename F>
ReflectResult ReflectFieldBody( R& r, ReflectedTypeInfo<R>& info, u16 id, F& f, StaticString const& name )
{
    return ReflectFieldBody( r, info, id, f, name, {} );
}
#define FIELD( id, f, ... )                 ReflectFieldBody( r, _reflectInfo, id, d.f, #f,     ##__VA_ARGS__ )
#define FIELD_NAMED( id, f, name, ... )     ReflectFieldBody( r, _reflectInfo, id, d.f, name,   ##__VA_ARGS__ )
#define FIELD_LOCAL( id, f, ... )           ReflectFieldBody( r, _reflectInfo, id, f,   #f,     ##__VA_ARGS__ )



#define REFLECT(...)      \
    template <typename R> \
    ReflectResult Reflect( R& r, __VA_ARGS__& d )

#define REFLECT_T(...)      \
    template <typename R, typename T> \
    ReflectResult Reflect( R& r, __VA_ARGS__& d )

// These don't work with FIELDs
#define REFLECT_SPECIAL_R(reflector, ...)      \
    ReflectResult Reflect( reflector& r, __VA_ARGS__& d )

#define REFLECT_SPECIAL_RT(reflector, ...)      \
    template <typename T> \
    ReflectResult Reflect( reflector& r, __VA_ARGS__& d )

#define REFLECT_SPECIAL_RW(reflector, ...)      \
    template <bool RW> \
    ReflectResult Reflect( reflector<RW>& r, __VA_ARGS__& d )

#define REFLECT_SPECIAL_RWT(reflector, ...)      \
    template <bool RW, typename T> \
    ReflectResult Reflect( reflector<RW>& r, __VA_ARGS__& d )


#define DEFINE_ADAPTER(name)         \
    template <typename I, typename O>  \
    struct name                        \
    {                                  \
        I& in;                         \
        O out;                         \
                                       \
        name( I& d ) : in( d ) , out{} \
        {}                             \
    };


REFLECT_T( EnumStruct<T> )
{
    T& e = *(T*)&d;

    using ValueType = T::ValueType;
    static_assert( !std::is_same< ValueType, EnumStruct<T>::InvalidValueType >(), "EnumStruct has no declared values to serialize" );

    ValueType v;
    STATIC_IF( r.IsWriting )
    {
        v = e.Value();
    }

    ReflectResult ret = Reflect( r, v );
    if( !ret )
        return ret;

    STATIC_IF( r.IsReading )
    {
        // TODO TEST TEST TEST !!!
        // TODO What happens if we use a struct value?
        e = T::FromValue( v );
    }

    return ReflectOk;
}

