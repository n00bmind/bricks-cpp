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


// Hash functions
//

// TODO See if we can extract a 'clean' version of XXH3 (for 128bits) from https://github.com/Cyan4973/xxHash/blob/dev/xxhash.h
// and add it to the bench for comparison with the rest


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

INLINE uint64_t getblock64 ( const uint64_t * p, sz i )
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
void MurmurHash3_x64_128 ( const void * key, const sz len,
                           const uint32_t seed, void * out )
{
  const uint8_t * data = (const uint8_t*)key;
  const sz nblocks = len / 16;

  uint64_t h1 = seed;
  uint64_t h2 = seed;

  const uint64_t c1 = BIG_CONSTANT(0x87c37b91114253d5);
  const uint64_t c2 = BIG_CONSTANT(0x4cf5ad432745937f);

  //----------
  // body

  const uint64_t * blocks = (const uint64_t *)(data);

  for(sz i = 0; i < nblocks; i++)
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
u64 MurmurHash3_x64_64( void const* buffer, sz len )
{
    u64 out[2];
    MurmurHash3_x64_128( buffer, len, 0, out );

    return out[0];
}


// Taken from https://create.stephan-brumme.com/xxhash/
/// XXHash (64 bit), based on Yann Collet's descriptions, see http://cyan4973.github.io/xxHash/
/** How to use:
    uint64_t myseed = 0;
    XXHash64 myhash(myseed);
    myhash.add(pointerToSomeBytes,     numberOfBytes);
    myhash.add(pointerToSomeMoreBytes, numberOfMoreBytes); // call add() as often as you like to ...
    // and compute hash:
    uint64_t result = myhash.hash();
    // or all of the above in one single line:
    uint64_t result2 = XXHash64::hash(mypointer, numBytes, myseed);
    Note: my code is NOT endian-aware !
**/
class XXHash64
{
public:
  /// create new XXHash (64 bit)
  /** @param seed your seed value, even zero is a valid seed **/
  explicit XXHash64(uint64_t seed)
  {
    state[0] = seed + Prime1 + Prime2;
    state[1] = seed + Prime2;
    state[2] = seed;
    state[3] = seed - Prime1;
    bufferSize  = 0;
    totalLength = 0;
  }

  /// add a chunk of bytes
  /** @param  input  pointer to a continuous block of data
      @param  length number of bytes
      @return false if parameters are invalid / zero **/
  bool add(const void* input, sz length)
  {
    // no data ?
    if (!input || length == 0)
      return false;

    totalLength += length;
    // byte-wise access
    const unsigned char* data = (const unsigned char*)input;

    // unprocessed old data plus new data still fit in temporary buffer ?
    if (bufferSize + length < MaxBufferSize)
    {
      // just add new data
      while (length-- > 0)
        buffer[bufferSize++] = *data++;
      return true;
    }

    // point beyond last byte
    const unsigned char* stop      = data + length;
    const unsigned char* stopBlock = stop - MaxBufferSize;

    // some data left from previous update ?
    if (bufferSize > 0)
    {
      // make sure temporary buffer is full (16 bytes)
      while (bufferSize < MaxBufferSize)
        buffer[bufferSize++] = *data++;

      // process these 32 bytes (4x8)
      process(buffer, state[0], state[1], state[2], state[3]);
    }

    // copying state to local variables helps optimizer A LOT
    uint64_t s0 = state[0], s1 = state[1], s2 = state[2], s3 = state[3];
    // 32 bytes at once
    while (data <= stopBlock)
    {
      // local variables s0..s3 instead of state[0]..state[3] are much faster
      process(data, s0, s1, s2, s3);
      data += 32;
    }
    // copy back
    state[0] = s0; state[1] = s1; state[2] = s2; state[3] = s3;

    // copy remainder to temporary buffer
    bufferSize = (uint64_t)(stop - data);
    for (uint64_t i = 0; i < bufferSize; i++)
      buffer[i] = data[i];

    // done
    return true;
  }

  /// get current hash
  /** @return 64 bit XXHash **/
  uint64_t hash() const
  {
    // fold 256 bit state into one single 64 bit value
    uint64_t result;
    if (totalLength >= MaxBufferSize)
    {
      result = rotateLeft(state[0],  1) +
               rotateLeft(state[1],  7) +
               rotateLeft(state[2], 12) +
               rotateLeft(state[3], 18);
      result = (result ^ processSingle(0, state[0])) * Prime1 + Prime4;
      result = (result ^ processSingle(0, state[1])) * Prime1 + Prime4;
      result = (result ^ processSingle(0, state[2])) * Prime1 + Prime4;
      result = (result ^ processSingle(0, state[3])) * Prime1 + Prime4;
    }
    else
    {
      // internal state wasn't set in add(), therefore original seed is still stored in state2
      result = state[2] + Prime5;
    }

    result += totalLength;

    // process remaining bytes in temporary buffer
    const unsigned char* data = buffer;
    // point beyond last byte
    const unsigned char* stop = data + bufferSize;

    // at least 8 bytes left ? => eat 8 bytes per step
    for (; data + 8 <= stop; data += 8)
      result = rotateLeft(result ^ processSingle(0, *(uint64_t*)data), 27) * Prime1 + Prime4;

    // 4 bytes left ? => eat those
    if (data + 4 <= stop)
    {
      result = rotateLeft(result ^ (*(uint32_t*)data) * Prime1,   23) * Prime2 + Prime3;
      data  += 4;
    }

    // take care of remaining 0..3 bytes, eat 1 byte per step
    while (data != stop)
      result = rotateLeft(result ^ (*data++) * Prime5,            11) * Prime1;

    // mix bits
    result ^= result >> 33;
    result *= Prime2;
    result ^= result >> 29;
    result *= Prime3;
    result ^= result >> 32;
    return result;
  }


  /// combine constructor, add() and hash() in one static function (C style)
  /** @param  input  pointer to a continuous block of data
      @param  length number of bytes
      @param  seed your seed value, e.g. zero is a valid seed
      @return 64 bit XXHash **/
  static uint64_t hash(const void* input, sz length, uint64_t seed)
  {
      XXHash64 hasher(seed);
      hasher.add(input, length);
      return hasher.hash();
  }
  static uint64_t hash(const void* input, sz length)
  {
      return hash( input, length, 0 );
  }

private:
  /// magic constants :-)
  static const uint64_t Prime1 = 11400714785074694791ULL;
  static const uint64_t Prime2 = 14029467366897019727ULL;
  static const uint64_t Prime3 =  1609587929392839161ULL;
  static const uint64_t Prime4 =  9650029242287828579ULL;
  static const uint64_t Prime5 =  2870177450012600261ULL;

  /// temporarily store up to 31 bytes between multiple add() calls
  static const uint64_t MaxBufferSize = 31+1;

  uint64_t      state[4];
  unsigned char buffer[MaxBufferSize];
  uint64_t      bufferSize;
  uint64_t      totalLength;

  /// rotate bits, should compile to a single CPU instruction (ROL)
  static inline uint64_t rotateLeft(uint64_t x, unsigned char bits)
  {
    return (x << bits) | (x >> (64 - bits));
  }

  /// process a single 64 bit value
  static inline uint64_t processSingle(uint64_t previous, uint64_t input)
  {
    return rotateLeft(previous + input * Prime2, 31) * Prime1;
  }

  /// process a block of 4x4 bytes, this is the main part of the XXHash32 algorithm
  static inline void process(const void* data, uint64_t& state0, uint64_t& state1, uint64_t& state2, uint64_t& state3)
  {
    const uint64_t* block = (const uint64_t*) data;
    state0 = processSingle(state0, block[0]);
    state1 = processSingle(state1, block[1]);
    state2 = processSingle(state2, block[2]);
    state3 = processSingle(state3, block[3]);
  }
};


// Inlined from https://github.com/jandrewrogers/MetroHash/blob/master/src/metrohash128.h & .cpp
class MetroHash128
{
public:
    static const uint32_t bits = 128;

    // Constructor initializes the same as Initialize()
    MetroHash128(const uint64_t seed=0)
    {
        Initialize(seed);
    }

    // Initializes internal state for new hash with optional seed
    void Initialize(const uint64_t seed=0)
    {
        // initialize internal hash registers
        state.v[0] = (static_cast<uint64_t>(seed) - k0) * k3;
        state.v[1] = (static_cast<uint64_t>(seed) + k1) * k2;
        state.v[2] = (static_cast<uint64_t>(seed) + k0) * k2;
        state.v[3] = (static_cast<uint64_t>(seed) - k1) * k3;

        // initialize total length of input
        bytes = 0;
    }

    // Update the hash state with a string of bytes. If the length
    // is sufficiently long, the implementation switches to a bulk
    // hashing algorithm directly on the argument buffer for speed.
    void Update(const uint8_t * buffer, const uint64_t length)
    {
        const uint8_t * ptr = reinterpret_cast<const uint8_t*>(buffer);
        const uint8_t * const end = ptr + length;

        // input buffer may be partially filled
        if (bytes % 32)
        {
            uint64_t fill = 32 - (bytes % 32);
            if (fill > length)
                fill = length;

            memcpy(input.b + (bytes % 32), ptr, static_cast<size_t>(fill));
            ptr   += fill;
            bytes += fill;

            // input buffer is still partially filled
            if ((bytes % 32) != 0) return;

            // process full input buffer
            state.v[0] += read_u64(&input.b[ 0]) * k0; state.v[0] = rotate_right(state.v[0],29) + state.v[2];
            state.v[1] += read_u64(&input.b[ 8]) * k1; state.v[1] = rotate_right(state.v[1],29) + state.v[3];
            state.v[2] += read_u64(&input.b[16]) * k2; state.v[2] = rotate_right(state.v[2],29) + state.v[0];
            state.v[3] += read_u64(&input.b[24]) * k3; state.v[3] = rotate_right(state.v[3],29) + state.v[1];
        }

        // bulk update
        bytes += (end - ptr);
        while (ptr <= (end - 32))
        {
            // process directly from the source, bypassing the input buffer
            state.v[0] += read_u64(ptr) * k0; ptr += 8; state.v[0] = rotate_right(state.v[0],29) + state.v[2];
            state.v[1] += read_u64(ptr) * k1; ptr += 8; state.v[1] = rotate_right(state.v[1],29) + state.v[3];
            state.v[2] += read_u64(ptr) * k2; ptr += 8; state.v[2] = rotate_right(state.v[2],29) + state.v[0];
            state.v[3] += read_u64(ptr) * k3; ptr += 8; state.v[3] = rotate_right(state.v[3],29) + state.v[1];
        }

        // store remaining bytes in input buffer
        if (ptr < end)
            memcpy(input.b, ptr, (size_t)(end - ptr));
    }   

    // Constructs the final hash and writes it to the argument buffer.
    // After a hash is finalized, this instance must be Initialized()-ed
    // again or the behavior of Update() and Finalize() is undefined.
    void Finalize(uint8_t * const hash)
    {
        // finalize bulk loop, if used
        if (bytes >= 32)
        {
            state.v[2] ^= rotate_right(((state.v[0] + state.v[3]) * k0) + state.v[1], 21) * k1;
            state.v[3] ^= rotate_right(((state.v[1] + state.v[2]) * k1) + state.v[0], 21) * k0;
            state.v[0] ^= rotate_right(((state.v[0] + state.v[2]) * k0) + state.v[3], 21) * k1;
            state.v[1] ^= rotate_right(((state.v[1] + state.v[3]) * k1) + state.v[2], 21) * k0;
        }

        // process any bytes remaining in the input buffer
        const uint8_t * ptr = reinterpret_cast<const uint8_t*>(input.b);
        const uint8_t * const end = ptr + (bytes % 32);

        if ((end - ptr) >= 16)
        {
            state.v[0] += read_u64(ptr) * k2; ptr += 8; state.v[0] = rotate_right(state.v[0],33) * k3;
            state.v[1] += read_u64(ptr) * k2; ptr += 8; state.v[1] = rotate_right(state.v[1],33) * k3;
            state.v[0] ^= rotate_right((state.v[0] * k2) + state.v[1], 45) * k1;
            state.v[1] ^= rotate_right((state.v[1] * k3) + state.v[0], 45) * k0;
        }

        if ((end - ptr) >= 8)
        {
            state.v[0] += read_u64(ptr) * k2; ptr += 8; state.v[0] = rotate_right(state.v[0],33) * k3;
            state.v[0] ^= rotate_right((state.v[0] * k2) + state.v[1], 27) * k1;
        }

        if ((end - ptr) >= 4)
        {
            state.v[1] += read_u32(ptr) * k2; ptr += 4; state.v[1] = rotate_right(state.v[1],33) * k3;
            state.v[1] ^= rotate_right((state.v[1] * k3) + state.v[0], 46) * k0;
        }

        if ((end - ptr) >= 2)
        {
            state.v[0] += read_u16(ptr) * k2; ptr += 2; state.v[0] = rotate_right(state.v[0],33) * k3;
            state.v[0] ^= rotate_right((state.v[0] * k2) + state.v[1], 22) * k1;
        }

        if ((end - ptr) >= 1)
        {
            state.v[1] += read_u8 (ptr) * k2; state.v[1] = rotate_right(state.v[1],33) * k3;
            state.v[1] ^= rotate_right((state.v[1] * k3) + state.v[0], 58) * k0;
        }

        state.v[0] += rotate_right((state.v[0] * k0) + state.v[1], 13);
        state.v[1] += rotate_right((state.v[1] * k1) + state.v[0], 37);
        state.v[0] += rotate_right((state.v[0] * k2) + state.v[1], 13);
        state.v[1] += rotate_right((state.v[1] * k3) + state.v[0], 37);

        bytes = 0;

        // do any endian conversion here

        memcpy(hash, state.v, 16);
    }

    // A non-incremental function implementation. This can be significantly
    // faster than the incremental implementation for some usage patterns.
    static void Hash(const uint8_t * buffer, const uint64_t length, uint8_t * const hash, const uint64_t seed=0)
    {
        const uint8_t * ptr = reinterpret_cast<const uint8_t*>(buffer);
        const uint8_t * const end = ptr + length;

        uint64_t v[4];

        v[0] = (static_cast<uint64_t>(seed) - k0) * k3;
        v[1] = (static_cast<uint64_t>(seed) + k1) * k2;

        if (length >= 32)
        {
            v[2] = (static_cast<uint64_t>(seed) + k0) * k2;
            v[3] = (static_cast<uint64_t>(seed) - k1) * k3;

            do
            {
                v[0] += read_u64(ptr) * k0; ptr += 8; v[0] = rotate_right(v[0],29) + v[2];
                v[1] += read_u64(ptr) * k1; ptr += 8; v[1] = rotate_right(v[1],29) + v[3];
                v[2] += read_u64(ptr) * k2; ptr += 8; v[2] = rotate_right(v[2],29) + v[0];
                v[3] += read_u64(ptr) * k3; ptr += 8; v[3] = rotate_right(v[3],29) + v[1];
            }
            while (ptr <= (end - 32));

            v[2] ^= rotate_right(((v[0] + v[3]) * k0) + v[1], 21) * k1;
            v[3] ^= rotate_right(((v[1] + v[2]) * k1) + v[0], 21) * k0;
            v[0] ^= rotate_right(((v[0] + v[2]) * k0) + v[3], 21) * k1;
            v[1] ^= rotate_right(((v[1] + v[3]) * k1) + v[2], 21) * k0;
        }

        if ((end - ptr) >= 16)
        {
            v[0] += read_u64(ptr) * k2; ptr += 8; v[0] = rotate_right(v[0],33) * k3;
            v[1] += read_u64(ptr) * k2; ptr += 8; v[1] = rotate_right(v[1],33) * k3;
            v[0] ^= rotate_right((v[0] * k2) + v[1], 45) * k1;
            v[1] ^= rotate_right((v[1] * k3) + v[0], 45) * k0;
        }

        if ((end - ptr) >= 8)
        {
            v[0] += read_u64(ptr) * k2; ptr += 8; v[0] = rotate_right(v[0],33) * k3;
            v[0] ^= rotate_right((v[0] * k2) + v[1], 27) * k1;
        }

        if ((end - ptr) >= 4)
        {
            v[1] += read_u32(ptr) * k2; ptr += 4; v[1] = rotate_right(v[1],33) * k3;
            v[1] ^= rotate_right((v[1] * k3) + v[0], 46) * k0;
        }

        if ((end - ptr) >= 2)
        {
            v[0] += read_u16(ptr) * k2; ptr += 2; v[0] = rotate_right(v[0],33) * k3;
            v[0] ^= rotate_right((v[0] * k2) + v[1], 22) * k1;
        }

        if ((end - ptr) >= 1)
        {
            v[1] += read_u8 (ptr) * k2; v[1] = rotate_right(v[1],33) * k3;
            v[1] ^= rotate_right((v[1] * k3) + v[0], 58) * k0;
        }

        v[0] += rotate_right((v[0] * k0) + v[1], 13);
        v[1] += rotate_right((v[1] * k1) + v[0], 37);
        v[0] += rotate_right((v[0] * k2) + v[1], 13);
        v[1] += rotate_right((v[1] * k3) + v[0], 37);

        // do any endian conversion here

        memcpy(hash, v, 16);
    }

    static u64 Hash64( const void* buffer, const sz length )
    {
        u8 out[16];
        Hash( (u8*)buffer, (u64)length, out );
        return *(u64*)out;
    }

    static u64 Hash64Inc( const void* buffer, const sz length )
    {
        u8 out[16];
        MetroHash128 h;
        h.Update( (u8*)buffer, (u64)length );
        h.Finalize( out );
        return *(u64*)out;
    }

private:
    // rotate right idiom recognized by most compilers
    inline static uint64_t rotate_right(uint64_t v, unsigned k)
    {
        return (v >> k) | (v << (64 - k));
    }

    // unaligned reads, fast and safe on Nehalem and later microarchitectures
    inline static uint64_t read_u64(const void * const ptr)
    {
        return static_cast<uint64_t>(*reinterpret_cast<const uint64_t*>(ptr));
    }

    inline static uint64_t read_u32(const void * const ptr)
    {
        return static_cast<uint64_t>(*reinterpret_cast<const uint32_t*>(ptr));
    }

    inline static uint64_t read_u16(const void * const ptr)
    {
        return static_cast<uint64_t>(*reinterpret_cast<const uint16_t*>(ptr));
    }

    inline static uint64_t read_u8 (const void * const ptr)
    {
        return static_cast<uint64_t>(*reinterpret_cast<const uint8_t *>(ptr));
    }

    static const uint64_t k0 = 0xC83A91E1;
    static const uint64_t k1 = 0x8648DBDB;
    static const uint64_t k2 = 0x7BDEC03B;
    static const uint64_t k3 = 0x2F5870A5;
    
    struct { uint64_t v[4]; } state;
    struct { uint8_t b[32]; } input;
    uint64_t bytes;
};



inline u32 Hash32( void const* buffer, sz len )
{
    u8 out[16];
    MetroHash128::Hash( (u8*)buffer, (u64)len, out );
    return *(u32*)out;
}

inline u64 Hash64( void const* buffer, sz len )
{
    u8 out[16];
    MetroHash128::Hash( (u8*)buffer, (u64)len, out );
    return *(u64*)out;
}

inline void Hash128( void const* buffer, sz len, u8* out )
{
    MetroHash128::Hash( (u8*)buffer, (u64)len, out );
}

// Incremental version
struct HashBuilder
{
    MetroHash128 h;
};

inline void HashAdd( HashBuilder* hash, void const* buffer, sz len )
{
    hash->h.Update( (u8*)buffer, (u64)len );
}

inline u32 Hash32( HashBuilder* hash )
{
    u8 out[16];
    hash->h.Finalize( out );
    return *(u32*)out;
}

inline u64 Hash64( HashBuilder* hash )
{
    u8 out[16];
    hash->h.Finalize( out );
    return *(u64*)out;
}

inline void Hash128( HashBuilder* hash, u8* out )
{
    hash->h.Finalize( out );
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
constexpr u64 CompileTimeHash64( const char* data, sz n, u64 basis )
{
    return n == 0 ? basis : CompileTimeHash64( data + 1, n - 1, ( basis ^ (data)[0] ) * 0x100000001b3ULL );
}
constexpr u64 CompileTimeHash64( const char* data, sz n )
{
    return CompileTimeHash64( data, n, 0xcbf29ce484222325ULL );
}

constexpr u32 CompileTimeHash32( const char* data, sz n )
{
    return (u32)(CompileTimeHash64( data, n, 0xcbf29ce484222325ULL ) & U32MAX);
}

template<size_t N>
constexpr u64 CompileTimeHash64( const char (&str)[N] )
{
    return CompileTimeHash64( str, N - 1 );
}

template<size_t N>
constexpr u32 CompileTimeHash32( const char (&str)[N] )
{
    return (u32)(CompileTimeHash64( str ) & U32MAX);
}

// TODO Disable these for char const* (string literals)
#if 1
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
#endif

// TODO This should be doable in C++ 20 using bit_cast
// See https://stackoverflow.com/questions/74137475/c17-constexpr-byte-iteration-for-trivial-types
// I just keep getting 'no matching overloaded function found' though
#if 0
template<typename T>
constexpr u64 CompileTimeHash64( const T& data )
{
    static_assert( std::has_unique_object_representations_v<T>, "Type must be trivially copyable to be able to hash it at compile time" );
    //return CompileTimeHash64( &std::bit_cast<char[sizeof(T)]>( data ), sizeof(T) );
    auto obj = std::bit_cast< std::array<std::byte, sizeof(T)>, T >( data );
    return CompileTimeHash64( obj.begin(), obj.size() );
}

template<typename T>
constexpr u32 CompileTimeHash32( const T& data )
{
    return (u32)CompileTimeHash64( data );
}
#endif

#define COMPTIME_HASH(...) \
    ForceCompileTime<u64, CompileTimeHash64(__VA_ARGS__)>::value

#define COMPTIME_HASH_32(...) \
    ForceCompileTime<u32, CompileTimeHash32(__VA_ARGS__)>::value


