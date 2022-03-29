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
INLINE bool IsPowerOf2( T value )
{
    return value > 0 && (value & (value - 1)) == 0;
}

INLINE u32 NextPowerOf2( u32 value )
{
    if( IsPowerOf2( value ) )
        return value;

    u32 result = 0;

#if _MSC_VER
    unsigned long msbPosition;
    if( _BitScanReverse( &msbPosition, value ) )
    {
        result = 1u << (msbPosition + 1);
    }
#else
    u32 leadingZeros = _lzcnt_u32( value );
    if( leadingZeros < 32 )
    {
        result = 1u << (32 - leadingZeros);
    }
#endif

    return result;
}

INLINE i32 NextPowerOf2( i32 value )
{
    u32 result = NextPowerOf2( U32( value ) );
    return I32( result );
}

#define CountShift(bits)  if( n >> bits ) { n >>= bits; result += bits; }
INLINE int Log2( i32 n )
{
    if( n <= 0 )
        return -1;

#if COMPILER_MSVC
    unsigned long result;
    _BitScanReverse( &result, (u32)n );
#elif COMPILER_LLVM
    NOT_IMPLEMENTED
#else
    u32 result = 0;
    CountShift(16);
    CountShift(8);
    CountShift(4);
    CountShift(2);
    CountShift(1);
#endif

    return (int)result;
}

INLINE int Log2( i64 n )
{
    if( n <= 0 )
        return -1;

#if COMPILER_MSVC
    unsigned long result;
    _BitScanReverse64( &result, (u64)n );
#elif COMPILER_LLVM
    NOT_IMPLEMENTED
#else
    u32 result = 0;
    CountShift(32);
    CountShift(16);
    CountShift(8);
    CountShift(4);
    CountShift(2);
    CountShift(1);
#endif

    return (int)result;
}
#undef CountShift

// Windows looves defining macros with common names
#undef Yield
INLINE void Yield()
{
    _mm_pause();
}
