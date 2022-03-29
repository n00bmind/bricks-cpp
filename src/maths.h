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

INLINE sz
AlignUp( sz size, sz alignment )
{
    ASSERT( IsPowerOf2( alignment ) );
    sz result = (size + (alignment - 1)) & ~(alignment - 1);
    return result;
}

INLINE void*
AlignUp( const void* address, sz alignment )
{
    ASSERT( IsPowerOf2( alignment ) );
    void* result = (void*)(((sz)address + (alignment - 1)) & ~(alignment - 1));
    return result;
}


#if defined(_MSC_VER)

#define ROTL64(x,y)	_rotl64(x,y)
#define BIG_CONSTANT(x) (x)

// Other compilers
#else

INLINE uint64_t rotl64 ( uint64_t x, int8_t r )
{
  return (x << r) | (x >> (64 - r));
}
#define ROTL64(x,y)	rotl64(x,y)
#define BIG_CONSTANT(x) (x##LLU)

#endif // !defined(_MSC_VER)

INLINE uint64_t getblock64 ( const uint64_t * p, int i )
{
  return p[i];
}

INLINE uint64_t fmix64 ( uint64_t k )
{
  k ^= k >> 33;
  k *= BIG_CONSTANT(0xff51afd7ed558ccd);
  k ^= k >> 33;
  k *= BIG_CONSTANT(0xc4ceb9fe1a85ec53);
  k ^= k >> 33;

  return k;
}

// Taken from https://github.com/aappleby/smhasher/blob/master/src/MurmurHash3.cpp
void MurmurHash3_x64_128 ( const void * key, const int len,
                           const uint32_t seed, void * out )
{
  const uint8_t * data = (const uint8_t*)key;
  const int nblocks = len / 16;

  uint64_t h1 = seed;
  uint64_t h2 = seed;

  const uint64_t c1 = BIG_CONSTANT(0x87c37b91114253d5);
  const uint64_t c2 = BIG_CONSTANT(0x4cf5ad432745937f);

  //----------
  // body

  const uint64_t * blocks = (const uint64_t *)(data);

  for(int i = 0; i < nblocks; i++)
  {
    uint64_t k1 = getblock64(blocks,i*2+0);
    uint64_t k2 = getblock64(blocks,i*2+1);

    k1 *= c1; k1  = ROTL64(k1,31); k1 *= c2; h1 ^= k1;

    h1 = ROTL64(h1,27); h1 += h2; h1 = h1*5+0x52dce729;

    k2 *= c2; k2  = ROTL64(k2,33); k2 *= c1; h2 ^= k2;

    h2 = ROTL64(h2,31); h2 += h1; h2 = h2*5+0x38495ab5;
  }

  //----------
  // tail

  const uint8_t * tail = (const uint8_t*)(data + nblocks*16);

  uint64_t k1 = 0;
  uint64_t k2 = 0;

  switch(len & 15)
  {
  case 15: k2 ^= ((uint64_t)tail[14]) << 48;
  case 14: k2 ^= ((uint64_t)tail[13]) << 40;
  case 13: k2 ^= ((uint64_t)tail[12]) << 32;
  case 12: k2 ^= ((uint64_t)tail[11]) << 24;
  case 11: k2 ^= ((uint64_t)tail[10]) << 16;
  case 10: k2 ^= ((uint64_t)tail[ 9]) << 8;
  case  9: k2 ^= ((uint64_t)tail[ 8]) << 0;
           k2 *= c2; k2  = ROTL64(k2,33); k2 *= c1; h2 ^= k2;

  case  8: k1 ^= ((uint64_t)tail[ 7]) << 56;
  case  7: k1 ^= ((uint64_t)tail[ 6]) << 48;
  case  6: k1 ^= ((uint64_t)tail[ 5]) << 40;
  case  5: k1 ^= ((uint64_t)tail[ 4]) << 32;
  case  4: k1 ^= ((uint64_t)tail[ 3]) << 24;
  case  3: k1 ^= ((uint64_t)tail[ 2]) << 16;
  case  2: k1 ^= ((uint64_t)tail[ 1]) << 8;
  case  1: k1 ^= ((uint64_t)tail[ 0]) << 0;
           k1 *= c1; k1  = ROTL64(k1,31); k1 *= c2; h1 ^= k1;
  };

  //----------
  // finalization

  h1 ^= len; h2 ^= len;

  h1 += h2;
  h2 += h1;

  h1 = fmix64(h1);
  h2 = fmix64(h2);

  h1 += h2;
  h2 += h1;

  ((uint64_t*)out)[0] = h1;
  ((uint64_t*)out)[1] = h2;
}

u32 Hash32( void const* key, const int len )
{
    u32 buf[4];
    MurmurHash3_x64_128( key, len, 0x1337C0D3, buf );

    return buf[0];
}

u64 Hash64( void const* key, const int len )
{
    u64 buf[2];
    MurmurHash3_x64_128( key, len, 0x1337C0D3, buf );

    return buf[0];
}


/*
 :: Some findings from the compile time hash experiments ::
    - The STL is a major clusterfuck, even for quick experiments like this one. Never again touch it with a 10-foot pole.
    - constexpr by itself is also a big disaster, as you still need to be very careful to assign everything to a constexpr variable
      or do template trickery to ensure the compiler is actually doing what the word implies, even in Release!
      (or use C++ 14 and 'consteval', WTF!?)
    - Putting a template in front of the constexpr function seems to _decrease_ compilation times, I assume the template must be acting
      as a cache of the final expression for each string length, hence saving a ton of compile time expression evaluations
      (and there are tons of string with the same length in the test body).
    - Using the ForceCompileTime trick also seems to decrease compilation times a little, no explanation for this one, as there are no
      repeating words in the test body, so I don't know what could be cached here.
*/

// Fairly typical FNV1-a loop in recursive form
// Number of collisions is low enough (0 in a test body of 100K+) that this can be used as a 1-to-1 id for short name strings
// (assuming we still check for them ofc!)
// https://mikejsavage.co.uk/blog/cpp-tricks-compile-time-string-hashing.html
constexpr u64 CompileTimeHash64( const u8* data, size_t n, u64 basis = 0xcbf29ce484222325ULL )
{
    return n == 0 ? basis : CompileTimeHash64( data + 1, n - 1, ( basis ^ data[0] ) * 0x100000001b3ULL );
}

constexpr u32 CompileTimeHash32( const u8* data, size_t n, u64 basis = 0xcbf29ce484222325ULL )
{
    return (u32)(CompileTimeHash64( data, n, basis ) & U32MAX);
}

template<size_t N>
constexpr u64 CompileTimeHash64( const char (&str)[N] )
{
    return CompileTimeHash64( (u8*)str, N - 1 );
}

template<size_t N>
constexpr u32 CompileTimeHash32( const char (&str)[N] )
{
    return (u32)(CompileTimeHash64( str ) & U32MAX);
}

// Taken from https://stackoverflow.com/a/12996028/2151254
template<typename T, typename std::enable_if_t<sizeof( T ) <= 8>* = nullptr>
constexpr u64 CompileTimeHash64( T data )
{
    u64 x = (u64)data;
    x = (x ^ (x >> 30)) * 0xbf58476d1ce4e5b9ULL;
    x = (x ^ (x >> 27)) * 0x94d049bb133111ebULL;
    x = x ^ (x >> 31);
    return x;
}

template<typename T, typename std::enable_if_t<sizeof( T ) <= 4>* = nullptr>
constexpr u32 CompileTimeHash32( T data )
{
    u32 x = (u32)data;
    x = ((x >> 16) ^ x) * 0x45d9f3b;
    x = ((x >> 16) ^ x) * 0x45d9f3b;
    x = (x >> 16) ^ x;
    return x;
}

// There just seems to be no way of doing this in this wonderful language
#if 0
template<typename T>
constexpr u64 CompileTimeHash64( const T& data )
{
    return CompileTimeHash64( (u8*)&data, sizeof(T) );
}

template<typename T>
constexpr u32 CompileTimeHash32( const T& data )
{
    return (u32)CompileTimeHash64( data );
}
#endif

#define CONSTEVAL_HASH(...) \
    ForceCompileTime<u64, CompileTimeHash64(__VA_ARGS__)>::value

#define CONSTEVAL_HASH_32(...) \
    ForceCompileTime<u32, CompileTimeHash32(__VA_ARGS__)>::value


