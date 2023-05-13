/*
The MIT License

Copyright (c) 2021 Oscar Peñas Pariente <n00bmindr0b0t@gmail.com>

Permission is hereby granted, free of charge, to any person obtaining a copy of
this software and associated documentation files (the "Software"), to deal in
the Software without restriction, including without limitation the rights to
use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of
the Software, and to permit persons to whom the Software is furnished to do so,
subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/
#pragma once

#if NON_UNITY_BUILD
#include <stdint.h>
#include <math.h>
#endif

//
// Common definitions
//

#define internal static
#define persistent static


//
// Compiler stuff
//

#if defined(__clang__) || defined(__GNUC__)   // Put this first so we account for clang-cl too

#define COMPILER_LLVM 1

#if defined(__amd64__) || defined(__x86_64__)
    #define ARCH_X64 1
#elif defined(__arm__)
    #define ARCH_ARM 1
#endif

#elif _MSC_VER

#define COMPILER_MSVC 1

#if defined(_M_X64) || defined(_M_AMD64)
    #define ARCH_X64 1
#elif defined(_M_ARM)
    #define ARCH_ARM 1
#endif

#else

#error Compiler not supported!

#endif //__clang__


#if COMPILER_MSVC
#define LIB_EXPORT extern "C" __declspec(dllexport)
#else
#define LIB_EXPORT extern "C" __attribute__((visibility("default")))
#endif

#if COMPILER_MSVC
    #define INLINE __forceinline
    #if __cplusplus > 201703L || _MSVC_LANG > 201703L
        #define INLINE_LAMBDA [[msvc::forceinline]]
    #else
        #define INLINE_LAMBDA 
    #endif
#else
    #define INLINE inline __attribute__((always_inline))
    #define INLINE_LAMBDA __attribute__((always_inline))
#endif


// TODO Some POSIX version of this
#define TRAP __debugbreak()

#if CONFIG_RELEASE
#define ASSERT(expr, ...) ((void)0)
#else
#define ASSERT(expr, ...) \
    ((void)( !(expr) \
             && (GetGlobalAssertHandler()( __FILE__, __LINE__, IF_ELSE( HAS_ARGS(__VA_ARGS__) )( __VA_ARGS__ )( #expr ) ), 1) \
             && (TRAP, 0) ))
#endif

#define ASSERT_HANDLER(name) void name( const char* file, int line, const char* msg, ... )
typedef ASSERT_HANDLER(AssertHandlerFunc);

// Use a getter so we access the same value from anywhere in the app
AssertHandlerFunc* GetGlobalAssertHandler();
void SetGlobalAssertHandler( AssertHandlerFunc* f );

#if !CONFIG_RELEASE
#define NOT_IMPLEMENTED ASSERT(!"NotImplemented")
#else
#define NOT_IMPLEMENTED NotImplemented!!!
#endif

#define INVALID_CODE_PATH ASSERT(!"InvalidCodePath");
#define INVALID_DEFAULT_CASE default: { INVALID_CODE_PATH; } break;

#if CONFIG_RELEASE
#define DEBUGBREAK(expr) ((void)0)
#else
// TODO Some POSIX version of this
#define DEBUGBREAK(expr) ((void)(expr && IsDebuggerPresent() && (__debugbreak(), 1)))
#endif

#define SIZEOF(s) ((sz)sizeof(s))
#define OFFSETOF(type, member) Sz( (uintptr_t)&(((type *)0)->member) )
#define ARRAYCOUNT(array) Sz( sizeof(array) / sizeof((array)[0]) )

// Quickly check size of a struct at compile time by forcing an error msg that includes it
// NOTE Must be placed at global / namespace scope
#define CHECKSIZE( ... ) \
    template<int> struct CompileTimeSize; \
    CompileTimeSize<sizeof( __VA_ARGS__ )> _##__VA_ARGS__;

#if 0
#define STR(s) _STR(s)
#define _STR(s) #s
#endif

#define KILOBYTES(value) ((value)*1024)
#define MEGABYTES(value) (KILOBYTES(value)*1024)
#define GIGABYTES(value) (MEGABYTES(value)*1024LL)

#define COPY(source, dest) memcpy( &dest, &source, sizeof(dest) )
#define SET(dest, value) memset( &dest, value, sizeof(dest) )
#define ZERO(dest) memset( &dest, 0, sizeof(dest) )
#define EQUAL(source, dest) (memcmp( &source, &dest, sizeof(source) ) == 0)

#define COPYP(source, dest, size) memcpy( dest, source, SizeT( size ) )
#define SETP(dest, value, size) memset( dest, value, SizeT( size ) )
#define ZEROP(dest, size) memset( dest, 0, SizeT( size ) )
#define EQUALP(source, dest, size) (memcmp( source, dest, SizeT( size ) ) == 0)

// Do a placement new on any variable with simpler syntax
#define INIT(var) new (&(var)) typename std::remove_reference<decltype(var)>::type
// NOTE Don't use with a pointer to a base class, as apparently this may inhibit virtual destructor calls
#define DEINIT(var) var.~decltype(var)()

#define __CONCAT(x, y) x ## y
#define CONCAT(x, y) __CONCAT(x, y)

// NOTE May cause trouble for stuff used at global scope!
#define UNIQUE(x) CONCAT(x, __LINE__)


// Force actual compile time execution of a constexpr function (similar to 'consteval')
// USE SPARINGLY
#if 0
template <typename T, T F>
struct ForceCompileTime
{
    static constexpr T value = F;
};
#define CONSTEVAL(type, funcCall) \
    ForceCompileTime<type, funcCall>::value
#else
// C++17 version that doesnt introduce any symbols
// See https://artificial-mind.net/blog/2020/11/14/cpp17-consteval
// Usage: int result = CONSTEVAL<ConstexprLog2( 32 )>;
// TODO Test using the string hash tests!
template <auto F>
static constexpr auto CONSTEVAL = F;
#endif

// Marker for a compile-time if, in case we either decide to go full nuts and use c++17, or find a decent substitute
// NOTE msvc has an outstanding bug going on for a decade which causes the __cplusplus macro to always return a really old value
#if __cplusplus >= 201703L || _MSVC_LANG >= 201703L
    #define IF if constexpr
#else
    #define IF if
#endif


typedef int8_t   i8;
typedef int16_t  i16;
typedef int32_t  i32;
typedef int64_t  i64;
typedef int64_t  sz;

typedef uint8_t  u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;

typedef float    f32;
typedef double   f64;


#define I8MIN INT8_MIN
#define I8MAX INT8_MAX
#define U8MAX UINT8_MAX
#define I16MAX INT16_MAX
#define I16MIN INT16_MIN
#define I32MAX INT32_MAX
#define I32MIN INT32_MIN
#define U16MAX UINT16_MAX
#define U32MAX UINT32_MAX
#define U64MAX UINT64_MAX // ULLONG_MAX?
#define I64MAX INT64_MAX
#define I64MIN INT64_MIN

#define F32MAX FLT_MAX
#define F32MIN FLT_MIN
#define F32INF INFINITY
#define F64MAX DBL_MAX
#define F64MIN DBL_MIN
#define F64INF (f64)INFINITY


typedef std::atomic<bool> atomic_bool;
typedef std::atomic<i32> atomic_i32;
typedef std::atomic<i64> atomic_i64;
typedef std::atomic<u64> atomic_u64;

#define LOAD_RELAXED() load( std::memory_order_relaxed )
#define LOAD_ACQUIRE() load( std::memory_order_acquire )
#define STORE_RELAXED(x) store( x, std::memory_order_relaxed )
#define STORE_RELEASE(x) store( x, std::memory_order_release )
#define COMPARE_EXCHANGE_ACQREL(exp, des) compare_exchange_weak( exp, des, std::memory_order_acq_rel )

template <typename T>
INLINE T Min( T a, T b )
{
    return a < b ? a : b;
}

template <typename T>
INLINE T Max( T a, T b )
{
    return a > b ? a : b;
}

template <typename T>
INLINE T Clamp( T v, T min, T max )
{
    return Min( max, Max( min, v ) );
}

// TODO See how to turn these into actual constexprs by using a constexpr-compatible assert expression
// TODO (may need to use C's assert?)
// https://akrzemi1.wordpress.com/2017/05/18/asserts-in-constexpr-functions/
// https://ericniebler.com/2014/09/27/assert-and-constexpr-in-cxx11/
// https://andreasfertig.blog/2018/08/use-of-the-comma-operator/

INLINE constexpr i8
I8( i32 value )
{
    ASSERT( I8MIN <= value && value <= I8MAX );
    return (i8)value;
}

INLINE constexpr u8
U8( u32 value )
{
    ASSERT( value <= U8MAX );
    return (u8)value;
}

INLINE constexpr u8
U8( i32 value )
{
    ASSERT( value >= 0 && value <= U8MAX );
    return (u8)value;
}

INLINE constexpr i16
I16( i32 value )
{
    ASSERT( I16MIN <= value && value <= I16MAX );
    return (i16)value;
}

INLINE constexpr u16
U16( i64 value )
{
    ASSERT( 0 <= value && value <= U16MAX );
    return (u16)value;
}

INLINE constexpr u16
U16( f64 value )
{
    ASSERT( 0 <= value && value <= U16MAX );
    return (u16)value;
}

INLINE constexpr i32
I32( sz value )
{
    ASSERT( I32MIN <= value && value <= I32MAX );
    return (i32)value;
}

INLINE constexpr i32
I32( f32 value )
{
    ASSERT( (f32)I32MIN <= value && value <= (f32)I32MAX );
    return (i32)value;
}

INLINE constexpr i32
I32( f64 value )
{
    ASSERT( I32MIN <= value && value <= I32MAX );
    return (i32)value;
}

INLINE constexpr i32
I32( u64 value )
{
    ASSERT( value <= (u64)I32MAX );
    return (i32)value;
}

INLINE constexpr i32
I32( u32 value )
{
    ASSERT( value <= (u32)I32MAX );
    return (i32)value;
}

INLINE constexpr u32
U32( i32 value )
{
    ASSERT( value >= 0 );
    return (u32)value;
}

INLINE constexpr u32
U32( u64 value )
{
    ASSERT( value <= U32MAX );
    return (u32)value;
}

INLINE constexpr u32
U32( i64 value )
{
    ASSERT( 0 <= value && value <= U32MAX );
    return (u32)value;
}

INLINE constexpr u32
U32( f64 value )
{
    ASSERT( 0 <= value && value <= U32MAX );
    return (u32)value;
}

INLINE constexpr sz
Sz( size_t value )
{
    ASSERT( value <= (size_t)I64MAX );
    return (sz)value;
}

INLINE constexpr size_t
SizeT( sz value )
{
    ASSERT( value >= 0 );
    return (size_t)value;
}

/////     BUFFER VIEW    /////

template <typename T = u8>
struct Buffer
{
    T* data;
    sz length;

public:
    Buffer()
        : data( nullptr )
        , length( 0 )
    {}

    template <typename SrcT>
    explicit Buffer( SrcT* data_, i64 length_ )
        : data( data_ )
        , length( length_ )
    {}

    // FIXME enable_if T is same size
    template <typename SrcT>
    explicit Buffer( Buffer<SrcT> const& other )
        : data( (T*)other.data )
        , length( other.length )
    {}

    // Convert from any static array of a compatible type
    // NOTE The array itself must be an lvalue, since by definition Buffers only reference data
    template <size_t N>
    Buffer( T (&data_)[N] )
        : data( data_ )
        , length( (sz)N )
    {}


    explicit INLINE operator T const*() const { return data; }
    explicit INLINE operator T*() { return data; }

    INLINE T& operator []( sz offset ) { ASSERT( offset < length ); return *(data + offset); }
    INLINE T const& operator []( sz offset ) const { ASSERT( offset < length ); return *(data + offset); }

    INLINE T*          begin()         { return data; }
    INLINE const T*    begin() const   { return data; }
    INLINE T*          end()           { return data + length; }
    INLINE const T*    end() const     { return data + length; }

    INLINE sz          Size() const    { return length * SIZEOF(T); }
    INLINE bool        Valid() const   { return data && length; }


    // Return how many items were actually copied
    INLINE sz CopyTo( T* buffer, sz itemCount, sz startOffset = 0 ) const
    {
        sz itemsToCopy = Min( itemCount, length - startOffset );
        COPYP( data + startOffset, buffer, itemsToCopy * SIZEOF(T) );

        return itemsToCopy;
    }

    // Return how many items were actually copied
    INLINE sz CopyFrom( T* buffer, sz itemCount, sz startOffset )
    {
        sz itemsToCopy = Min( itemCount, length - startOffset );
        COPYP( buffer, data + startOffset, itemsToCopy * SIZEOF(T) );

        return itemsToCopy;
    }
};

using StringBuffer = Buffer<char const>;

// This is the least horrible thing I could come up with ¬¬
#define BUFFER(T, ...)                    \
[]()                                      \
{                                         \
    static T literal[] = { __VA_ARGS__ }; \
    return Buffer<T>( literal );          \
}()
    


/////     ENUM STRUCT    /////
// TODO Combine with the ideas in https://blog.paranoidcoding.com/2010/11/18/discriminated-unions-in-c.html to create a similar
// TAGGED_UNION for discriminated unions

/* Usage:

struct V
{
    u64 iVal;
};

// NOTE Composite values must be declared inside {}
#define VALUES(x) \
    x(None,         "a",  { 42u } ) \
    x(Animation,    "b",  { 42u } ) \
    x(Landscape,    "c",  { 42u } ) \
    x(Audio,        "d",  { 42u } ) \
    x(Network,      "e",  { 42u } ) \
    x(Scripting,    "f",  { 42u } ) \

ENUM_STRUCT_WITH_NAMES_VALUES(MemoryTag, V, VALUES)
#undef VALUES

int main()
{
    // Can be used as a normal enum
    int index = MemoryTag::Audio;
    
    // But also
    MemoryTag t = MemoryTag::Audio;
    ASSERT_EQ( t.Index(), index );
    ASSERT_EQ( (int)t, index );

    char const* name = t.Name();

    for( int i = 0; i < MemoryTag::itemCount; ++i )
    {
        MemoryTag::ValueType const& v = MemoryTag::values[i];
        LOG( "%s is: %llu, %d\n", MemoryTag::names[i], v.uVal, v.iVal );
    }
}
*/

#define _ENUM_ENTRY(x, ...)                     x,

#define _ENUM_NAME_1(x)                         #x,
#define _ENUM_NAME_2a(x, n)                     n,
#define _ENUM_NAME_2b(x, ...)                   #x,
#define _ENUM_NAME_3(x, n, ...)                 n,

#define _ENUM_NAME_HASH_1(x)                    CompileTimeHash64( #x ),
#define _ENUM_NAME_HASH_2a(x, n)                CompileTimeHash64( n ),
#define _ENUM_NAME_HASH_2b(x, ...)              CompileTimeHash64( #x ),
#define _ENUM_NAME_HASH_3(x, n, ...)            CompileTimeHash64( n ),

#define _ENUM_NAME_CASE(x, ...)                 case nameHashes[x]:                     \
                                                    if( StringsEqual( names[x], name ) ) \
                                                    {                                   \
                                                        result = x;                     \
                                                        match = true;                   \
                                                    }                                   \
                                                break;

// TODO We'd ideally remove values entirely so any code using them (serialization!) errors out at compile time
#define _ENUM_VALUE_1(x)                        {},
#define _ENUM_VALUE_2a(x, n)                    {},
#define _ENUM_VALUE_2b(x, ...)                  __VA_ARGS__,
#define _ENUM_VALUE_3(x, n, ...)                __VA_ARGS__,

#define _ENUM_VALUE_HASH_1(x)                   (u64)x,                                 // Use index as a dummy value
#define _ENUM_VALUE_HASH_2a(x, n)               (u64)x,                                 // Use index as a dummy value
// FIXME This works only for up to 64 bit values or strings
// TODO We may just want to return the value itself for integer values, since we'll be using consecutive values
// which may mean more chance for the compiler to optimize FromValue() into a jump table
#define _ENUM_VALUE_HASH_2b(x, ...)             CompileTimeHash64( __VA_ARGS__ ),
#define _ENUM_VALUE_HASH_3(x, n, ...)           CompileTimeHash64( __VA_ARGS__ ),
//#define _ENUM_VALUE_HASH(x, ...)                CompileTimeHash64( (char*)&values[x], sizeof(ValueType) ),

#define _ENUM_VALUE_CASE(x, ...)                case valueHashes[x]:         \
                                                    if( values[x] == value ) \
                                                    {                        \
                                                        result = x;          \
                                                        match = true;        \
                                                    }                        \
                                                break;

//#define _ENUM_ARGS(...)                         { __VA_ARGS__ }
//#define _ENUM_ITEM_1(x)                         { #x, {}, x },
//#define _ENUM_ITEM_2a(x, n)                     {  n, {}, x },
//#define _ENUM_ITEM_2b(x, v)                     { #x, _ENUM_ARGS v, x },
//#define _ENUM_ITEM_3(x, n, v)                   {  n, _ENUM_ARGS v, x },

// TODO What stuff can we possibly move to the parent and take out of the macro?
template <typename T> struct EnumStruct
{
    struct InvalidValueType
    {
        bool operator ==( InvalidValueType const& rhs ) const { return false; }
    };

protected:
    constexpr EnumStruct() {};
};

#define _CREATE_ENUM(enumName, xItemList, xSelector, valueType) \
class enumName : public EnumStruct<enumName>                                        \
{                                                                                   \
    i32 index;                                                                      \
                                                                                    \
public:                                                                             \
    enum Enum : i32                                                                 \
    {                                                                               \
        xItemList(_ENUM_ENTRY)                                                      \
    };                                                                              \
                                                                                    \
    using EnumTypeName = enumName;                                                  \
    using ValueType = valueType;                                                    \
                                                                                    \
                                                                                    \
    constexpr enumName() : index( itemCount )                                       \
    {}                                                                              \
    constexpr enumName( int index_ ) : index( index_ )                              \
    {                                                                               \
        ASSERT( index >= 0 && index < itemCount, #enumName " index out of range" ); \
    }                                                                               \
                                                                                    \
    constexpr bool IsValid() const { return index >= 0 && index < itemCount; }      \
                                                                                    \
    INLINE constexpr operator int() { return index; }                               \
    INLINE constexpr bool operator ==( enumName const& other ) const                \
    { return index == other.index; }                                                \
    INLINE constexpr bool operator ==( Enum const& other ) const                    \
    { return index == (int)other; }                                                 \
    INLINE constexpr bool operator <( enumName const& other ) const                 \
    { return index < other.index; }                                                 \
    INLINE constexpr bool operator <=( enumName const& other ) const                \
    { return index <= other.index; }                                                \
    INLINE constexpr bool operator >( enumName const& other ) const                 \
    { return index > other.index; }                                                 \
    INLINE constexpr bool operator >=( enumName const& other ) const                \
    { return index >= other.index; }                                                \
                                                                                    \
    INLINE constexpr void operator ++()                                             \
    { index = (index < itemCount - 1) ? index + 1 : 0; }                            \
                                                                                    \
    INLINE constexpr int Index()  const { return index; }             \
    INLINE constexpr char const* Name()  const { return names[index]; }             \
    INLINE constexpr valueType const& Value() const { return values[index]; }       \
                                                                                    \
    static enumName FromName( char const* name )                                    \
    {                                                                               \
        enumName result;                                                            \
        bool match = false;                                                         \
        u64 nHash = CompileTimeHash64( name );                                      \
        switch( nHash )                                                             \
        {                                                                           \
            xItemList(_ENUM_NAME_CASE)                                              \
        }                                                                           \
        ASSERT( match, "Undeclared name in " #enumName );                           \
        return result;                                                              \
    }                                                                               \
                                                                                    \
    template <typename T = valueType,                                               \
              std::enable_if_t< !std::is_same<T, InvalidValueType>() >* = nullptr>  \
    static enumName FromValue( valueType const& value )                             \
    {                                                                               \
        enumName result;                                                            \
        bool match = false;                                                         \
        u64 vHash = CompileTimeHash64( value );                                     \
        switch( vHash )                                                             \
        {                                                                           \
            xItemList(_ENUM_VALUE_CASE)                                             \
        }                                                                           \
        ASSERT( match, "Undeclared value in " #enumName );                          \
        return result;                                                              \
    }                                                                               \
                                                                                    \
    static constexpr char const* names[] =                                          \
    {                                                                               \
        xItemList( CAT(_ENUM_NAME_, xSelector) )                                                       \
    };                                                                              \
    static constexpr u64 nameHashes[] =                                             \
    {                                                                               \
        xItemList( CAT(_ENUM_NAME_HASH_, xSelector) )                                                  \
    };                                                                              \
    static constexpr valueType values[] =                                           \
    {                                                                               \
        xItemList( CAT(_ENUM_VALUE_, xSelector) )                                                      \
    };                                                                              \
    static constexpr u64 valueHashes[] =                                            \
    {                                                                               \
        xItemList( CAT(_ENUM_VALUE_HASH_, xSelector) )                                                      \
    };                                                                              \
    static constexpr sz itemCount = ARRAYCOUNT(names);                              \
};                                                                  

#define ENUM_STRUCT(enumName, xItemList)                                _CREATE_ENUM(enumName, xItemList, 1,  InvalidValueType)
#define ENUM_STRUCT_WITH_NAMES(enumName, xItemList)                     _CREATE_ENUM(enumName, xItemList, 2a, InvalidValueType)
#define ENUM_STRUCT_WITH_VALUES(enumName, valueType, xItemList)         _CREATE_ENUM(enumName, xItemList, 2b, valueType)
#define ENUM_STRUCT_WITH_NAMES_VALUES(enumName, valueType, xItemList)   _CREATE_ENUM(enumName, xItemList, 3,  valueType)


/////     DEFER    /////
#define DEFER(...)                                                  \
auto UNIQUE(_deferred_func_) = [&]() INLINE_LAMBDA { __VA_ARGS__ }; \
struct UNIQUE(Deferred)                                             \
{                                                                   \
	INLINE ~UNIQUE(Deferred)()                                      \
	{                                                               \
		f();                                                        \
	}                                                               \
	decltype(UNIQUE(_deferred_func_)) f;                            \
};                                                                  \
UNIQUE(Deferred) UNIQUE(_deferred_) { UNIQUE(_deferred_func_) };





/////     CALLABLE    /////
// TODO Create a wrapper similar in spirit to Allocator + Will's Functors
// The wrapper just calls to operator() and the internal implementations are just objects with a byte array of a maximum size
// to store their "captures" (which are specified in their construction, each type decides whether by ref or value, etc)
// https://stackoverflow.com/questions/25985248/speed-of-bound-lambda-via-stdfunction-vs-operator-of-functor-struct
// https://devblogs.microsoft.com/oldnewthing/20200515-00/?p=103755
