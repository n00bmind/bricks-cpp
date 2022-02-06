#include <stdio.h>
#include <inttypes.h>
#include <string.h>

#include "../src/bricks_common_win32.inl"

/*
 :: SOME FINDINGS ::
    - The STL is a major clusterfuck, even for quick experiments like this one. Never again touch it with a 10-foot pole.
    - constexpr by itself is also a big disaster, as you still need to be very careful or do template trickery to ensure the compiler
      is actually doing what the word implies, even in Release! (or use c++ 14 and 'consteval', WTF!?)
    - Putting a template in front of the constexpr function seems to _decrease_ compilation times, I assume the template must be acting
      as a cache of the final expression for each string length, hence saving a ton of compile time expression evaluations
      (and there are tons of string with the same length in the test body).
    - Using the ForceCompileTime trick also seems to decrease compilation times, no explanation for this one, as there are no repeating
      words, so I don't know what could be cached here.

*/

// Fixed constant to use as baseline
#define Hash0(x) 1234567890ULL


// awesome compile time "x65599" / sdbm hash: specialise on N=1 to stop it expanding to illegal N=0
// https://www.popekim.com/2012/01/compile-time-hash-string-generation.html
__forceinline constexpr u64 Hash1(const char (&str)[1]) { return str[0];}

template<size_t N>
__forceinline constexpr u64 Hash1(const char (&str)[N])
{
    typedef const char (&truncated_str)[N-1];
    return ( str[N-1] + ( 65599 * Hash1((truncated_str)str) ) );
}

template<size_t N>
__forceinline constexpr u32 Hash1_32(const char (&str)[N])
{
    return (u32)(Hash1( str ) & U32MAX);
}


// https://mikejsavage.co.uk/blog/cpp-tricks-compile-time-string-hashing.html
constexpr u64 Hash2( const char * str, size_t n, u64 basis = 0xcbf29ce484222325ULL )
{
    return n == 0 ? basis : Hash2( str + 1, n - 1, ( basis ^ str[ 0 ] ) * 0x100000001b3ULL );
}

template<size_t N>
constexpr u64 Hash2( const char (&str)[ N ] )
{
    return Hash2( str, N - 1 );
}

template<size_t N>
constexpr u32 Hash2_32( const char (&str)[N] )
{
    return (u32)(Hash2( str ) & U32MAX);
}


// https://simoncoenen.com/blog/programming/StaticReflection
constexpr u64 Hash3( const char* const str, const u64 value = 0xcbf29ce484222325ULL )
{
    return (str[0] == '\0') ? value : Hash3( &str[1], (value ^ u64(str[0])) * 0x100000001b3ULL );
}


// https://handmade.network/forums/t/1507-compile_time_string_hashing_with_c++_constexpr_vs._your_own_preprocessor
constexpr u64 Hash4( const char* string )
{
    u64 hash = 0xcbf29ce484222325ULL;
    while (*string)
    {
        hash = hash ^ (u64)(*string++);
        hash = hash * 0x100000001b3ULL;
    }

    return hash;
}

static constexpr u64 Hash5(char head, const char* tail, u64 hash)
{
    return head ? Hash5(tail[0], tail + 1, (hash ^ (u64)head) * 0x100000001b3ULL)
                : hash;
}

static constexpr u64 Hash5(const char* string)
{
    return Hash5(string[0], string + 1, 0xcbf29ce484222325ULL);
}



template <typename T, T F>
struct ForceCompileTime
{
    static constexpr T value = F;
};

#define CONSTEVAL_HASH(x) \
    ForceCompileTime<u64, Hash2(x)>::value

#define CONSTEVAL_HASH_32(x) \
    ForceCompileTime<u32, Hash2_32(x)>::value


int main()
{
    SetUnhandledExceptionFilter( Win32::ExceptionHandler );

    Logging::ChannelDecl channels[] =
    {
        { "Platform" },
    };
    InitGlobalPlatform( channels );

    bool found;
    int collisions = 0, total = 0;
    Hashtable<u64, char const*> map;

    double startTime = Core::AppTimeMillis();

#if 1
#define WORD(x)                              \
    map.PutIfNotFound( CONSTEVAL_HASH(x), nullptr, &found ); \
    total++;                                 \
    if( found )                              \
        collisions++;                        
#else
#define WORD(x)                              \
    map.PutIfNotFound( ForceCompileTime<u32, Hash2_32(x)>::value, nullptr, &found ); \
    total++;                                 \
    if( found )                              \
        collisions++;                        
#endif

#include "words.inl"

    double totalTime = Core::AppTimeMillis() - startTime;
    printf( "Total collisions: %d of %d (%.2f %%)\n", collisions, total, (float)collisions * 100.f / total );
    printf( "Execution time: %.2f millis\n", totalTime );
}

