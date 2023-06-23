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


// TODO Write copy/move constructors & assignments for everybody
// TODO Move Push() for everybody




/////     DYNAMIC ARRAY    /////

// Actually, a reference to an array of any size and type somewhere in memory, so more like a 'buffer view' as it's called in do-lang
// (although we have semantics for Push/Pop/Remove etc. so it is not read-only)

template <typename T, typename AllocType /*= Allocator*/>
struct Array
{
    T* data;
    i32 count;
    i32 capacity;

    AllocType* allocator;
    MemoryParams memParams;


#if 0
    static const Array<T, AllocType> Empty;
#endif

    Array()
        : data( nullptr )
        , count( 0 )
        , capacity( 0 )
        , allocator( nullptr )
        , memParams{}
    {}

    // NOTE All newly allocated arrays start empty
    // NOTE You cannot specify an AllocType for the instance and then not provide an allocator
    explicit Array( i32 capacity_, AllocType* alloc = CTX_ALLOC, MemoryParams params = Memory::NoClear() )
        : count( 0 )
        , capacity( capacity_ )
        , allocator( alloc )
        , memParams( params )
    {
        ASSERT( allocator );
        data = ALLOC_ARRAY( allocator, T, capacity, memParams );
    }

    // Initializing from a temporary array needs to allocate and copy
    // TODO For some reason, passing an array with a single int will always match the above constructor instead!
    // TODO This still doesnt work when we're inside a struct and trying to initialize with an 'initializer list'
    // Maybe try some of the stuff in https://www.foonathan.net/2016/12/fixing-initializer-list/
    template <size_t N>
    explicit Array( T (&&data_)[N], AllocType* alloc = CTX_ALLOC, MemoryParams params = Memory::NoClear() )
        : Array( (int)N, alloc, params )
    {
        ResizeToCapacity();
        COPYP( data_, data, capacity * SIZEOF(T) );
    }

    // NOTE Arrays initialized from a buffer have count set equal to their capacity by default
    explicit Array( T* buffer, i32 bufferLen, i32 initialCount = -1 )
        : data( buffer )
        , count( initialCount == -1 ? bufferLen : initialCount )
        , capacity( bufferLen )
        , allocator( nullptr )
        , memParams{}
    {
        ASSERT( count >= 0 && count <= capacity );
    }

    // Initialize from a Buffer of the same type (same as above)
    Array( Buffer<T> const& buffer )
        : Array( buffer.data, (int)buffer.length, (int)buffer.length )
    {}

    // Initialize from an l-value array of the same type (same as above)
    template <size_t N>
    Array( T (&data_)[N] )
        : Array( data_, (int)N, (int)N )
    {}

    Array( Array const& ) = default;
    void operator =( Array const& ) = delete;

    Array( Array&& other )
    {
        *this = std::move( other );
    }

    ~Array()
    {
        Destroy();
    }

    void Reset( i32 new_capacity, AllocType* new_allocator = CTX_ALLOC,
                MemoryParams new_params = Memory::NoClear(), bool clear = true )
    {
        ASSERT( new_allocator );
        T* new_data = ALLOC_ARRAY( new_allocator, T, new_capacity, new_params );

        if( data )
        {
            sz copy_size = Min( count, new_capacity ) * SIZEOF(T);
            COPYP( data, new_data, copy_size );
        }

        if( allocator )
            FREE( allocator, data, memParams );

        data      = new_data;
        count     = clear ? 0 : Min( count, new_capacity );
        capacity  = new_capacity;
        allocator = new_allocator;
        memParams = new_params;
    }

    void Destroy()
    {
        if( allocator )
            FREE( allocator, data, memParams );

        data = nullptr;
        count = 0;
        capacity = 0;
        allocator = nullptr;
        memParams = {};
    }


    void operator =( Array&& other )
    {
        data      = other.data;
        count     = other.count;
        capacity  = other.capacity;
        allocator = other.allocator;
        memParams = other.memParams;

        ZERO( other );
    }

    INLINE explicit operator Buffer<T>()
    {
        return Buffer<T>( data, count );
    }

    INLINE explicit operator bool() const
    {
        return data != nullptr && count != 0;
    }


    INLINE T*          begin()         { return data; }
    INLINE const T*    begin() const   { return data; }
    INLINE T*          end()           { return data + count; }
    INLINE const T*    end() const     { return data + count; }

    INLINE T&          First()         { ASSERT( count > 0 ); return data[0]; }
    INLINE T const&    First() const   { ASSERT( count > 0 ); return data[0]; }
    INLINE T&          Last()          { ASSERT( count > 0 ); return data[count - 1]; }
    INLINE T const&    Last() const    { ASSERT( count > 0 ); return data[count - 1]; }

    INLINE sz          Size() const    { return count * SIZEOF(T); }
    INLINE bool        Empty() const   { return count == 0; }

    void Resize( i32 new_count )
    {
        ASSERT( new_count >= 0 && new_count <= capacity );
        count = new_count;
    }

    void ResizeToCapacity()
    {
        count = capacity;
    }

    void Clear()
    {
        count = 0;
    }

    i32 Available() const
    {
        return capacity - count;
    }


    T& operator[]( int i )
    {
        ASSERT( i >= 0 && i < count );
        return data[i];
    }

    const T& operator[]( int i ) const
    {
        ASSERT( i >= 0 && i < count );
        return data[i];
    }

    // TODO Can we deduce whether we need to construct/destruct or we can skip it at compile time?
    // https://en.cppreference.com/w/cpp/named_req/PODType
    // https://en.cppreference.com/w/cpp/named_req/TrivialType
    // https://en.cppreference.com/w/cpp/named_req/TriviallyCopyable
    // https://en.cppreference.com/w/cpp/named_req/StandardLayoutType
    T* PushEmpty( bool clear = true )
    {
        ASSERT( count < capacity, "Array[%d] overflow", capacity );
        T* result = data + count++;
        if( clear )
            // TODO Call constructor or clear to zero depending on std::is_trivially_copyable(T)
            INIT( *result );

        return result;
    }

    T* Push( const T& item )
    {
        T* slot = PushEmpty( false );
        // TODO Call constructor or clear to zero depending on std::is_trivially_copyable(T)
        INIT( *slot )( item );

        return slot;
    }

    T* Push( T&& item )
    {
        T* slot = PushEmpty( false );
        INIT( *slot )( std::move(item) );

        return slot;
    }

    template <class... TInitArgs>
    T* PushInit( TInitArgs&&... args )
    {
        T* slot = PushEmpty( false );
        INIT( *slot )( args... );

        return slot;
    }

    void Remove( T* item )
    {
        ASSERT( item >= begin() && item < end() );

        T* last = &Last();
        if( item != last )
            *item = std::move( *last );

        last->~T();
        --count;
    }

    T Pop()
    {
        T result = Last();
        Remove( &Last() );

        return result;
    }

    T* Find( const T& item )
    {
        T* result = nullptr;
        for( int i = 0; i < count; ++i )
        {
            if( data[i] == item )
            {
                result = &data[i];
                break;
            }
        }

        return result;
    }

    T const* Find( const T& item ) const
    {
        T* slot = ((Array<T, AllocType>*)this)->Find( item );
        return slot;
    }

    bool Contains( T const& item ) const
    {
        T const* slot = Find( item );
        return slot != nullptr;
    }

    template <class Predicate>
    T* Find( Predicate&& p )
    {
        T* result = nullptr;
        for( int i = 0; i < count; ++i )
        {
            if( p( data[i] ) )
            {
                result = &data[i];
                break;
            }
        }

        return result;
    }

    template <class Predicate>
    T const* Find( Predicate&& p ) const
    {
        T* slot = ((Array<T, AllocType>*)this)->Find( p );
        return slot;
    }

    template <class Predicate>
    bool Contains( Predicate&& p ) const
    {
        T const* slot = Find( p );
        return slot != nullptr;
    }

    void Push( T const* buffer, int bufferLen )
    {
        ASSERT( Available() >= bufferLen );

        COPYP( buffer, data + count, bufferLen * SIZEOF(T) );
        count += bufferLen;
    }

    template <typename AllocType2 = Allocator>
    void Append( Array<T, AllocType2> const& array )
    {
        Push( array.data, array.count );
    }

    // Deep copy
    template <typename AllocType2 = Allocator>
    Array<T, AllocType2> Clone( AllocType2* alloc = nullptr ) const
    {
        Array<T, AllocType2> result( count, alloc ? alloc : CTX_ALLOC );
        result.ResizeToCapacity();
        COPYP( data, result.data, count * SIZEOF(T) );
        return result;
    }

    template <typename AllocType2 = Allocator>
    static Array<T, AllocType2> Clone( Array<T, AllocType2> const& other, AllocType2* alloc = nullptr )
    {
        Array<T, AllocType2> result( other.capacity, alloc ? alloc : CTX_ALLOC );
        result.count = other.count;
        COPYP( other.data, result.data, result.count * SIZEOF(T) );
        return result;
    }

    // Return how many items were actually copied
    INLINE sz CopyTo( T* buffer, sz itemCount, sz startOffset = 0 ) const
    {
        sz itemsToCopy = Min( itemCount, count - startOffset );
        COPYP( data + startOffset, buffer, itemsToCopy * SIZEOF(T) );

        return itemsToCopy;
    }

    // Return how many items were actually copied
    INLINE sz CopyFrom( T* buffer, sz itemCount, sz startOffset )
    {
        sz itemsToCopy = Min( itemCount, count - startOffset );
        COPYP( buffer, data + startOffset, itemsToCopy * SIZEOF(T) );

        return itemsToCopy;
    }

    template <typename AllocType2 = Allocator>
    void CopyTo( Array<T, AllocType2>* out ) const
    {
        ASSERT( out->capacity >= count );
        COPYP( data, out->data, count * SIZEOF(T) );
        out->count = count;
    }
};

// Helper functions to clone stuff into arrays without having to specify freaking template arguments everywhere
template <typename T, typename AllocType = Allocator>
Array<T, AllocType> ArrayClone( Array<T, AllocType> const& other, AllocType* allocator = nullptr )
{
    return Array<T, AllocType>::Clone( other, allocator );
}

template <typename T, typename AllocType = Allocator>
Array<T, AllocType> ArrayClone( Buffer<T> const& other, AllocType* allocator = nullptr )
{
    return Array<T, AllocType>::Clone( Array<T, AllocType>( other ), allocator );
}

#if 0
template <typename T, typename AllocType>
const Array<T, AllocType> Array<T, AllocType>::Empty = {};
#endif


/////     BUCKET ARRAY     /////

// Growable container that allocates its items in pages, or 'buckets', so no extra copying occurs whatsoever when new items get pushed.
// The buckets themselves are indexed in a linear array, which should stay reasonably hot when iterated often, and could easily
// be modified in a lock-free fashion for a potential multithreaded version.
// Items are always kept compact, so iteration remains really fast. Since bucket sizes are Po2, it can be iterated using a normal integer
// index or with the provided iterator (for new-style 'for each').

// TODO Benchmark this against std::vector and the compact_vector proposed in https://www.sebastiansylvan.com/post/space-efficient-rresizable-arrays/
// (the twitter thread https://twitter.com/andy_kelley/status/1516949533413892096?t=vEW63veywf4fqhfBfPRQSA&s=03 is quite interesting)
template <typename T, typename AllocType /*= Allocator*/>
struct BucketArray
{
    // TODO Do we wanna keep the count+capacity in the array of buckets or do we wanna put them inline with the bucket memory?
    // (more buckets per cache line vs. dereferencing each bucket block more often)
    struct Bucket
    {
        T* data;
        i32 count;
        i32 capacity;
    };

    // TODO Pack this down
    AllocType* allocator;
    Bucket* bucketBuffer;
    i32 bucketBufferCount;
    i32 bucketBufferCapacity;
    sz count;
    // TODO Remove freelist
    T* firstFree;
    i32 bucketCapacity;

    MemoryParams memParams;


    template <typename A>
    struct IteratorBase
    {
        A array;
        sz index;

        INLINE bool             operator ==( IteratorBase<A> const& rhs ) const { return array == rhs.array && index == rhs.index; }
        INLINE bool             operator !=( IteratorBase<A> const& rhs ) const { return array != rhs.array || index != rhs.index; }
        INLINE IteratorBase<A>& operator ++()                                   { ++index; return *this; }
        INLINE IteratorBase<A>  operator ++( int)                               { IteratorBase<A> result(*this); index++; return result; }
    };
    struct Iterator : public IteratorBase< BucketArray<T, AllocType>* >
    {
        INLINE T&               operator *() const                              { return (*array)[index]; }
    };
    struct ConstIterator : public IteratorBase< BucketArray<T, AllocType> const* >
    {
        INLINE T const&         operator *() const                              { return (*array)[index]; }
    };


    BucketArray( i32 bucketSize = 16, AllocType* alloc = CTX_ALLOC, MemoryParams params = Memory::NoClear() )
        : allocator( nullptr )
        , bucketBuffer( nullptr )
        , bucketBufferCount( 0 )
        , bucketBufferCapacity( 0 )
        , count( 0 )
        , firstFree( nullptr )
    {
        Reset( bucketSize, alloc, params );
    }

    ~BucketArray()
    {
        Destroy();
    }


    // Absolutely no idea why this requires double braces on MSVC
    INLINE Iterator         begin()          { return {{ this, 0 }}; }
    INLINE ConstIterator    begin() const    { return {{ this, 0 }}; }
    INLINE Iterator         end()            { return {{ this, count }}; }
    INLINE ConstIterator    end() const      { return {{ this, count }}; }

    INLINE T&          First()          { return (*this)[0]; }
    INLINE T const&    First() const    { return (*this)[0]; }
    INLINE T&          Last()           { return (*this)[count - 1]; }
    INLINE T const&    Last() const     { return (*this)[count - 1]; }

    INLINE sz          Size() const     { return count * SIZEOF(T); }
    INLINE sz          Capacity() const { return bucketBufferCount * bucketCapacity; }
    INLINE bool        Empty() const    { return count == 0; }


    INLINE void FindBucket( sz index, int* bucketIndex, int* indexInBucket ) const
    {
        const int bucketShift = Log2( bucketCapacity );
        *bucketIndex   = (int)(index >> bucketShift);
        *indexInBucket = (int)(index & (bucketCapacity - 1));
    }

    // Same as an Array, we don't allow gaps in between elements, so the given index acts as a mask of the bucket index + index in the bucket
    INLINE T& operator []( sz index )
    { 
        ASSERT( index >= 0 && index < count, "BucketArray[%d] out of bounds (%d)", index, count );

        int bucketIndex, indexInBucket;
        FindBucket( index, &bucketIndex, &indexInBucket );

        return bucketBuffer[ bucketIndex ].data[ indexInBucket ];
    }
    INLINE T const& operator []( sz index ) const    
    { 
        return (*(BucketArray<T, AllocType>*)this)[ index ];
    }

    INLINE Bucket* GetLastBucket()
    {
        return bucketBufferCount ? &bucketBuffer[bucketBufferCount - 1] : nullptr;
    }


    void Reset( i32 bucketSize = 16, AllocType* alloc = CTX_ALLOC, MemoryParams params = Memory::NoClear() )
    {
        Destroy();

        ASSERT( alloc );
        allocator = alloc;
        bucketCapacity = NextPowerOf2( bucketSize );
        memParams = params;

        ASSERT( IsPowerOf2( bucketCapacity ) );
        // We need a minimum size to be able to chain it to the freelist when retiring
        ASSERT( bucketCapacity * sizeof(T) >= sizeof(T*), "Bucket size too small" );
    }

    void Destroy()
    {
        for( int i = 0; i < bucketBufferCount; ++i )
            FREE( allocator, bucketBuffer[i].data, memParams );

        while( firstFree )
        {
            T* data = firstFree;
            firstFree = *(T**)&firstFree[0];

            FREE( allocator, data, memParams );
        }

        if( allocator )
            FREE( allocator, bucketBuffer, memParams );

        allocator = nullptr;
        bucketBuffer = nullptr;
        bucketBufferCount = 0;
        bucketBufferCapacity = 0;
        count = 0;
        firstFree = nullptr;
        bucketCapacity = 0;
        memParams = {};
    }

    void Reserve( sz capacity )
    {
        if( capacity <= Capacity() )
            return;

        int newBucketCount = (int)((capacity + bucketCapacity - 1) / bucketCapacity);
        GrowBucketBuffer( newBucketCount );

        for( int i = bucketBufferCount; i < newBucketCount; ++i )
            AllocBucket();
    }

private:
    // NOTE Separate any initialization calls so that only the relevant constructors
    // are required in the user type
    INLINE T* PushInternal()
    {
        Bucket* lastBucket = GetLastBucket();
        if( !lastBucket || lastBucket->count == lastBucket->capacity )
            lastBucket = AllocBucket();

        T* result = lastBucket->data + lastBucket->count++;
        count++;

        return result;
    }

public:
    T* PushEmpty()
    {
        T* slot = PushInternal();
        INIT( *slot );
        return slot;
    }

    T* Push( const T& item )
    {
        T* slot = PushInternal();
        INIT( *slot )( item );
        return slot;
    }

    T* Push( T&& item )
    {
        T* slot = PushInternal();
        INIT( *slot )( std::move(item) );
        return slot;
    }

    template <class... TInitArgs>
    T* PushInit( TInitArgs&&... args )
    {
        T* slot = PushInternal();
        INIT( *slot )( args... );

        return slot;
    }

    void PushEmpty( int itemCount, bool clear = true )
    {
        Bucket* lastBucket = GetLastBucket();
        if( !lastBucket || lastBucket->count == lastBucket->capacity )
            lastBucket = AllocBucket();

        int remaining = itemCount, itemsToAdd = 0;
        while( remaining > 0 )
        {
            itemsToAdd = Min( remaining, lastBucket->capacity - lastBucket->count );
            if( clear )
                ZEROP( lastBucket->data + lastBucket->count, itemsToAdd * SIZEOF(T) );

            lastBucket->count += itemsToAdd;
            remaining -= itemsToAdd;

            if( remaining )
                lastBucket = AllocBucket();
        }

        count += itemCount;
    }

    INLINE void Push( T const* buffer, sz bufferLen )
    {
        Bucket* lastBucket = GetLastBucket();
        if( !lastBucket || lastBucket->count == lastBucket->capacity )
            lastBucket = AllocBucket();

        sz remaining = bufferLen; int itemsToCopy = 0;
        while( remaining > 0 )
        {
            itemsToCopy = (int)Min( remaining, (sz)lastBucket->capacity - lastBucket->count );
            COPYP( buffer, lastBucket->data + lastBucket->count, itemsToCopy * SIZEOF(T) );

            lastBucket->count += itemsToCopy;
            buffer += itemsToCopy;
            remaining -= itemsToCopy;

            if( remaining )
                lastBucket = AllocBucket();
        }

        count += bufferLen;
    }

    T Pop()
    {
        Bucket* lastBucket = GetLastBucket();
        ASSERT( lastBucket );

        int idx = --lastBucket->count;
        T result = std::move(lastBucket->data[ idx ]);

        if( idx == 0 )
            RetireBucket( lastBucket );
        count--;

        return std::move(result);
    }

    // TODO Remove (swap)
    // TODO Insert
    // TODO RemoveOrdered

    void Clear()
    {
        // Add all buckets to the free list (backwards so its faster)
        // Leave bucket 0 in place since it's probably gonna be reused anyway
        for( int i = bucketBufferCount - 1; i > 0; --i )
            RetireBucket( bucketBuffer + i );

        if( bucketBufferCount > 0 )
            bucketBuffer[0].count = 0;
        count = 0;
    }


    // TODO Add Find
    // TODO Add LowerBound search (see Algorithm.h)

    // Return how many items were actually copied
    // FIXME All methods using memcpy should check if the type is trivially copyable!
    INLINE sz CopyTo( T* buffer, sz itemCount, sz startOffset = 0 ) const
    {
        ASSERT( startOffset >= 0 );

        int bucketIndex, indexInBucket;
        FindBucket( startOffset, &bucketIndex, &indexInBucket );

        T* bufferStart = buffer;
        Bucket const* b = bucketBuffer + bucketIndex;
        sz remaining = Min( itemCount, count - startOffset );
        while( remaining > 0 )
        {
            sz itemsToCopy = Min( (sz)b->count - indexInBucket, remaining );
            COPYP( b->data + indexInBucket, buffer, itemsToCopy * SIZEOF(T) );
            indexInBucket = 0;

            buffer += itemsToCopy;
            remaining -= itemsToCopy;
            ++b;
        }

        return buffer - bufferStart;
    }

    void MoveTo( T* buffer, sz itemCount )
    {
        for( int i = 0; i < bucketBufferCount; ++i )
        {
            Bucket& b = bucketBuffer[i];
            T* end = b.data + b.count;
            for( T* src = b.data; src < end; src++ )
            {
                *buffer = std::move( *src );
                buffer++;
            }
        }
    }

    template <typename AllocType2 = Allocator>
    void CopyTo( Array<T, AllocType2>* array ) const
    {
        ASSERT( count <= array->capacity );
        array->Resize( I32(count) );

        T* buffer = array->data;
        for( int i = 0; i < bucketBufferCount; ++i )
        {
            Bucket const& b = bucketBuffer[i];
            COPYP( b.data, buffer, b.count * SIZEOF(T) );
            buffer += b.count;
        }
    }

    // Copy on top of the existing elements, without altering the total count
    // Return how many were actually copied
    INLINE sz CopyFrom( T* buffer, sz itemCount, sz startOffset )
    {
        ASSERT( startOffset >= 0 );
        ASSERT( startOffset + itemCount <= count );

        int bucketIndex, indexInBucket;
        FindBucket( startOffset, &bucketIndex, &indexInBucket );

        T* bufferStart = buffer;
        Bucket const* b = bucketBuffer + bucketIndex;
        sz remaining = Min( itemCount, count - startOffset );
        while( remaining > 0 )
        {
            sz itemsToCopy = Min( (sz)bucketCapacity - indexInBucket, remaining );
            COPYP( buffer, b->data + indexInBucket, itemsToCopy * SIZEOF(T) );
            indexInBucket = 0;

            buffer += itemsToCopy;
            remaining -= itemsToCopy;
            ++b;
        }

        return buffer - bufferStart;
    }


    template <typename AllocType2 = Allocator>
    Array<T, AllocType2> ToArray( AllocType2* alloc = nullptr ) const
    {
        Array<T, AllocType2> result( I32(count), alloc ? alloc : CTX_ALLOC );
        result.ResizeToCapacity();
        CopyTo( &result );

        return result;
    }

    Array<Buffer<T>> const ToBufferArray() const
    {
        Array<Buffer<T>> result( bucketBufferCount );

        for( int i = 0; i < bucketBufferCount; ++i )
        {
            Bucket const& b = bucketBuffer[i];
            result.Push( Buffer<T>( b.data, b.count ) );
        }
        return result;
    }

    Array<Buffer<>> const ToRawBufferArray() const
    {
        Array<Buffer<>> result( bucketBufferCount );

        for( int i = 0; i < bucketBufferCount; ++i )
        {
            Bucket const& b = bucketBuffer[i];
            result.Push( Buffer<>( b.data, b.count * SIZEOF(T) ) );
        }
        return result;
    }


    u64 HashContents()
    {
        HashBuilder h;
        for( Bucket* b = bucketBuffer; b < bucketBuffer + bucketBufferCount; ++b )
            HashAdd( &h, b->data, b->count * SIZEOF(T) );

        // TODO I assume this returns 0 on an empty buffer.. how do we want to signal that?
        return Hash64( &h );
    }


private:
    Bucket* AllocBucket()
    {
        if( bucketBufferCount == bucketBufferCapacity )
            GrowBucketBuffer();

        Bucket* new_bucket   = &bucketBuffer[ bucketBufferCount++ ];
        new_bucket->data     = ALLOC_ARRAY( allocator, T, bucketCapacity, memParams );
        new_bucket->count    = 0;
        new_bucket->capacity = bucketCapacity;

        return new_bucket;
    }

    void RetireBucket( Bucket* b )
    {
        // If we're the last guy just don't
        if( b == bucketBuffer )
            return;

        // Stick a pointer to the next guy at the beginning of the buffer
        *(T**)&b->data[0] = firstFree;
        firstFree = b->data;

        // Move everybody following in the array of buckets forward
        sz idx = b - bucketBuffer;
        for( sz i = idx; i < bucketBufferCount - 1; ++i )
            bucketBuffer[i] = bucketBuffer[i + 1];

        bucketBuffer[ --bucketBufferCount ] = {};
    }

    void GrowBucketBuffer( int new_bucket_capacity = 0 )
    {
        new_bucket_capacity = Max( 4, new_bucket_capacity
            ? new_bucket_capacity : bucketBufferCapacity * 2 );

        Bucket* new_buffer = ALLOC_ARRAY( allocator, Bucket, new_bucket_capacity, memParams );
        COPYP( bucketBuffer, new_buffer, bucketBufferCount * SIZEOF(Bucket) );

        FREE( allocator, bucketBuffer, memParams );
        bucketBuffer = new_buffer;
        bucketBufferCapacity = new_bucket_capacity;
    }
};






// TODO There's two conflicting views on this datatype rn:
// 1 One would be that we want to keep it fast (both to iterate and to compact into an array), so we want to keep buckets compacted
//   at all times, where any item removed is swapped with the last one (or subsequent items are moved down) so that we can iterate
//   each bucket fast with just the pointer to the start of the bucket and its count.
// 2 The second view is that this is a datatype used mainly for storage, and so we want items to keep a stable address, and be able
//   to return an Idx/Handle to them. For that we'd need to keep items in place always, and keep something like an occupancy bitfield.
//
// Now, Part Pools in KoM have this same exact duality, and I think the concept of a roster used there can help, but used in the
// opposite sense it's currently being used there.. Instead of the roster being a way to iterate consecutively even in the presence
// of gaps in the pool, we'd much rather have the buckets be consecutive always (option 1) and use the roster as the thing that
// a Handle points to (for option 2).
//
// The smart thing to do is probably separate these two objectives in two separate datatypes: BucketArray tries to be as simple as
// possible and just compacts its items down inside each bucket, while PagedPool is composed of a BucketArray plus a roster that it
// uses to hand out Handles to any added items (complete with generation numbers etc.)

// TODO Rewrite more in the style of PagedPool in KoM (where pages are just a linear array of pointers) to make it faster to 
// iterate & access in line with the first objective above .. BUT compacting means we can never have an index
// Once you remove elements inside a bucket..
//   · How does that bucket get filled up again? (insertion)
//   · What happens when it's fully empty? (the array of page pointers gets rewritten)
//   · BUT if we can never ref. items directly why do we need/want a linear array of pages? (easier for the page array to stay hot, easier to multithread..)
//   · Will also need a RemoveOrdered for usage as a i.e. StringBuilder


// TODO It seems really that the stable referencing is the true differentiating factor, and hence there's two things here:
// - BucketArray that iterates fast by compacting but doesnt provide any means of referencing individual items
// - PagedPool that has an occupancy mask and provides Handles (a composed index to the page + index inside the page) with gen. numbers and even naked pointers but is slower to iterate
// We could then add:
// - CompactPool composed of a BucketArray + roster that both iterates fast and provides storable Handles in exchange for complexity
template <typename T, typename AllocType = Allocator>
struct OldBucketArray
{
    struct Bucket
    {
        Bucket *next;
        Bucket *prev;               // TODO Do we really use this?
        AllocType* allocator;

        T* data;
        i32 count;
        i32 capacity;
        MemoryParams memParams;

        Bucket()
            : next( nullptr )
            , prev( nullptr )
            , allocator( nullptr )
            , data( nullptr )
            , count( 0 )
            , capacity( 0 )
        {}

        Bucket( i32 capacity_, AllocType* alloc, MemoryParams memParams_ )
            : next( nullptr )
            , prev( nullptr )
            , allocator( alloc )
            , count( 0 )
            , capacity( capacity_ )
            , memParams( memParams_ )
        {
            ASSERT( capacity > 0 );
            ASSERT( allocator );
            data = ALLOC_ARRAY( allocator, T, capacity, memParams );
        }

        ~Bucket()
        {
            Clear();
            FREE( allocator, data, memParams );
        }

        bool Empty() const { return count == 0; }

        void Clear()
        {
            count = 0;
            next = prev = nullptr;
        }
    };

    template <typename V, typename B>
    struct IdxBase
    {
        B base;
        i32 index;

        IdxBase( B base_ = nullptr, i32 index_ = 0 )
            : base( base_ )
            , index( index_ )
        {}

        // FIXME Additionally check that base is not in the free list
        bool IsValid() const { return base && index >= 0 && index < base->count; }
        explicit operator bool() const { return IsValid(); }

        operator   V() const { ASSERT( IsValid() ); return base->data[index]; }
        V operator *() const { return (V)*this; }

        bool operator ==( IdxBase<V, B> const& other ) const { return base == other.base && index == other.index; }
        bool operator !=( IdxBase<V, B> const& other ) const { return !(*this == other); }


        void Next()
        {
            if( index < base->count )
                index++;
            // If there's no next bucket, stay at end()
            if( index == base->count && base->next )
            {
                base = base->next;
                index = 0;
            }
        }

        IdxBase& operator ++()      { Next(); return *this; }
        IdxBase  operator ++( int ) { IdxBase result(*this); Next(); return result; }

        void Prev()
        {
            if( index > 0 )
                index--;
            // If there's no previous bucket, stay at begin()
            if( index == 0 && base->prev )
            {
                base = base->prev;
                index = base->count - 1;
            }
        }

        IdxBase& operator --() { Prev(); return *this; }
        IdxBase operator --( int ) { IdxBase result(*this); Prev(); return result; }
    };

    using Idx = IdxBase<T&, Bucket*>;
    using ConstIdx = IdxBase<T const&, Bucket const*>;


    Bucket first;
    Bucket* last;
    Bucket* firstFree;

    AllocType* allocator;
    MemoryParams memParams;
    i32 count;


#if 0
    static const OldBucketArray<T> Empty;
#endif

    OldBucketArray()
        : last( nullptr )
        , firstFree( nullptr )
        , allocator( nullptr )
        , memParams{}
        , count( 0 )
    {}

    explicit OldBucketArray( i32 bucketSize, AllocType* alloc = CTX_ALLOC, MemoryParams params = Memory::NoClear() )
        : first( bucketSize, alloc, params )
        , last( &first )
        , firstFree( nullptr )
        , allocator( alloc )
        , memParams( params )
        , count( 0 )
    {
        ASSERT( allocator );
    }

    OldBucketArray( OldBucketArray<T>&& other )
        : first( other.first )
        , last( other.last )
        , firstFree( other.firstFree )
        , memParams( other.memoryParams )
        , count( other.count )
    {
        other.first = {};
        other.last = nullptr;
        other.firstFree = nullptr;
        other.memoryParams = {};
        other.count = 0;
    }

    ~OldBucketArray()
    {
        Clear();
        
        while( firstFree )
        {
            Bucket* b = firstFree;
            DELETE( allocator, b, Bucket, memParams );

            firstFree = firstFree->next;
        }
    }

    // Disallow implicit copying
    OldBucketArray( const OldBucketArray& ) = delete;
    OldBucketArray& operator =( const OldBucketArray& ) = delete;

    bool        Empty() const { return count == 0; }

    Idx         begin()       { return { &first, 0 }; }
    ConstIdx    begin() const { return { &first, 0 }; }
    Idx         end()         { return { last, last->count }; }
    ConstIdx    end()   const { return { last, last->count }; }

    Idx         Begin()       { return { &first, 0 }; }
    ConstIdx    Begin() const { return { &first, 0 }; }
    Idx         End()         { return { last, last->count }; }
    ConstIdx    End()   const { return { last, last->count }; }
    Idx         First()       { return { &first, 0 }; }
    ConstIdx    First() const { return { &first, 0 }; }
    Idx         Last()        { return { last, last->count - 1 }; }
    ConstIdx    Last()  const { return { last, last->count - 1 }; }

    T& operator[]( const Idx& idx ) { ASSERT( idx.IsValid() ); return (T&)idx; }
    const T& operator[]( const ConstIdx& idx ) const { ASSERT( idx.IsValid() ); return (T const&)idx; }

    template <typename AllocType2 = Allocator>
    bool operator ==( Array<T, AllocType2> const& other ) const
    {
        if( count != other.count)
            return false;

        int i = 0;
        auto idx = First(); 
        while( idx )
        {
            T const& e = *idx;
            if( e != other[i++] )
                return false;

            idx.Next();
        }

        return true;
    }

    void Clear()
    {
        if( last != &first )
        {
            // Place all chained buckets in the free list
            last->next = firstFree;
            firstFree = first.next;
        }

        first.Clear();
        count = 0;
        last = &first;
    }

    T* PushEmpty( bool clear = true )
    {
        if( last->count == last->capacity )
            AddEmptyBucket();

        count++;
        T* result = &last->data[last->count++];
        if( clear )
            // TODO Call constructor or clear to zero depending on std::is_trivially_copyable(T)
            INIT( *result );

        return result;
    }

    T* Push( const T& item )
    {
        T* slot = PushEmpty( false );
        // TODO Call constructor or clear to zero depending on std::is_trivially_copyable(T)
        INIT( *slot )( item );
        return slot;
    }

    // TODO Test
    // FIXME This is totally incompatible with the Idx idea
    void Remove( const Idx& index, T* itemOut = nullptr )
    {
        ASSERT( index.IsValid() );

        T* item = &(*index);
        if( itemOut )
            *itemOut = *item;

        Bucket* b = index.base;
        // If index is not pointing to last item in the bucket, swap it
        // FIXME This is totally incompatible with the Idx idea
        T* lastItem = &b->data[ b->count - 1 ];
        if( item != lastItem )
            *item = *lastItem;

        lastItem->~T();
        b->count--;

        if( b->count == 0 && b != &first )
        {
            // Empty now, so unlink it and place it at the beginning of the free list
            b->prev->next = b->next;
            b->next->prev = b->prev;

            // NOTE We don't maintain the prev pointer for stuff in the freelist as it seems pointless?
            b->prev = nullptr;
            b->next = firstFree;
            firstFree = b;
        }

        count--;
    }

    T Pop()
    {
        T result;
        Remove( Last(), &result );
        return result;
    }

    // Inclusive
    void PopUntil( const Idx& index )
    {
        while( index.IsValid() )
            Pop();
    }

    // TODO Untested
#if 0
    // Not particularly fast
    T const& At( int index ) const
    {
        ASSERT( index >= 0 && index < count );

        Bucket const* base = &first;
        const int bucketSize = first.count;

        while( index >= bucketSize )
        {
            index -= bucketSize;
            base = base->next;
        }

        return base->data[index];
    }
    T& At( int index )
    {
        return const_cast<OldBucketArray const*>(this)->At( index );
    }

    void Grow( int newCount )
    {
        // TODO This can be used to create gaps, which might be a problem in other situations?
        if( newCount > count )
        {
            Bucket const* base = &first;
            const int bucketSize = first.count;

            int index = newCount;
            while( index >= bucketSize )
            {
                index -= bucketSize;
                if( base == last )
                    AddEmptyBucket();
                base = base->next;
            }
        }
    }

    void Resize( int newCount )
    {
        if( newCount > count )
            Grow( newCount );
        //else
            // TODO Add invalidated buckets to freelist?

        count = newCount;
    }
#endif

    T* Find( T const& item )
    {
        T* result = nullptr;

        auto idx = First();
        while( idx )
        {
            if( cmp( item, *idx ) )
                break;

            idx.Next();
        }

        if( idx )
            result = &*idx;

        return result;
    }

    T const* Find( T const& item ) const
    {
        T* result = ((OldBucketArray<T, AllocType>*)this)->Find( item );
        return result;
    }

    bool Contains( T const& item ) const
    {
        T const* result = Find( item );
        return result != nullptr;
    }

    template <class Predicate>
    T* Find( Predicate&& p )
    {
        T* result = nullptr;

        auto idx = First();
        while( idx )
        {
            if( p( *idx ) )
                break;

            idx.Next();
        }

        if( idx )
            result = &*idx;

        return result;
    }

    template <class Predicate>
    T const* Find( Predicate&& p ) const
    {
        T* slot = ((OldBucketArray<T, AllocType>*)this)->Find( p );
        return slot;
    }

    template <class Predicate>
    bool Contains( Predicate&& p ) const
    {
        T const* slot = Find( p );
        return slot != nullptr;
    }


    void Push( T const* buffer, int bufferLen )
    {
        int totalCopied = 0;
        Bucket*& bucket = last;

        while( totalCopied < bufferLen )
        {
            int remaining = bufferLen - totalCopied;
            int available = bucket->capacity - bucket->count;

            int copied = Min( remaining, available );
            COPYP( buffer + totalCopied, bucket->data + bucket->count, copied * SIZEOF(T) );
            totalCopied += copied;

            bucket->count += copied;
            if( bucket->count == bucket->capacity )
                AddEmptyBucket();
        }

        count += totalCopied;
    }

    template <typename AllocType2 = Allocator>
    void Append( Array<T, AllocType2> const& array )
    {
        Push( array.data, array.count );
    }

    void CopyTo( T* buffer, sz bufferLen ) const
    {
        ASSERT( count <= bufferLen );

        const Bucket* bucket = &first;
        while( bucket )
        {
            COPYP( bucket->data, buffer, bucket->count * SIZEOF(T) );
            buffer += bucket->count;
            bucket = bucket->next;
        }
    }

    template <typename AllocType2 = Allocator>
    void CopyTo( Array<T, AllocType2>* array ) const
    {
        ASSERT( count <= array->capacity );
        array->Resize( count );

        T* buffer = array->data;
        const Bucket* bucket = &first;
        while( bucket )
        {
            COPYP( bucket->data, buffer, bucket->count * SIZEOF(T) );
            buffer += bucket->count;
            bucket = bucket->next;
        }
    }

    template <typename AllocType2 = Allocator>
    Array<T, AllocType2> ToArray( AllocType2* alloc = nullptr ) const
    {
        Array<T, AllocType2> result( count, alloc ? alloc : CTX_ALLOC );
        result.ResizeToCapacity();
        CopyTo( &result );

        return result;
    }

private:

    void AddEmptyBucket()
    {
        Bucket* newBucket;
        if( firstFree )
        {
            newBucket = firstFree;
            firstFree = firstFree->next;
            newBucket->Clear();
        }
        else
        {
            // TODO Inline this with the actual contents
            newBucket = NEW( allocator, Bucket, memParams )( first.capacity, allocator, memParams );
        }
        newBucket->prev = last;
        last->next = newBucket;

        last = newBucket;
    }
};

#if 0
template <typename T, typename AllocType>
const OldBucketArray<T, AllocType> OldBucketArray<T, AllocType>::Empty = {};
#endif


/////     STRING BUILDER     /////

// String builder to help compose Strings piece by piece
// NOTE This uses temporary memory by default!
struct StringBuilder
{
    BucketArray<char> buckets;

    explicit StringBuilder( Allocator* allocator = CTX_TMPALLOC )
        : buckets( 32, allocator )
    {}

    bool Empty() const { return buckets.count == 0; }

    void Clear()
    {
        buckets.Clear();
    }

    void Append( char const* str, int length = 0 )
    {
        if( !length )
            length = StringLength( str );

        // Not including null terminator
        buckets.Push( str, length );
    }

    // TODO No way to use this as it will always prefer the above!
    template <size_t N>
    void Append( char (&&str)[N] )
    {
        // Not including null terminator
        buckets.Push( str, N - 1 );
    }

    void AppendFormat( char const* fmt, ... )
    {
        va_list args;
        va_start( args, fmt );

        int n = 1 + vsnprintf( nullptr, 0, fmt, args );
        char* buf = ALLOC_ARRAY( CTX_TMPALLOC, char, n, Memory::NoClear() );

        // TODO Does this work??
        vsnprintf( buf, SizeT( n ), fmt, args );
        va_end( args );

        // TODO We probably want this struct to be made out of irregular buckets that are allocated exactly of the
        // length needed for each append (n above) so we don't need this extra copy
        buckets.Push( buf, n - 1 );
    }

    String ToString()
    {
        buckets.Push( '\0' );
        return String::Clone( buckets );
    }
    String ToStringTmp()
    {
        buckets.Push( '\0' );
        return String::CloneTmp( buckets );
    }
    char const* ToCStringTmp()
    {
        return ToStringTmp().c();
    }
};


/////     HASHTABLE     /////

/*
After watching how Bitwise handles open addressing for pointer hashtables, I think I want to keep separate data structures
for pointer-sized data vs. the rest, when generalizing this data structure to hold any type of data.

So, for pointers we'd have something like:
*/

struct PtrHashtable
{
    struct Item
    {
        void* key;
        void* value;
    };

    Item* items;
    i32 count;
    i32 capacity;
};

/*
We're trying to optimize for the "hole-in-1" approach here, assuming a sufficiently good hash function, and we'll also keep the
occupancy quite low by doing an ASSERT( count * 2 < capacity ) before each insertion, so for the few times that we don't hit
the correct entry on the first try, it makes sense to keep the values inlined with the keys, as the correct result will be hopefully
just a couple entries away and hence will still be in the same cache line, so we'll _hopefully_ still only have one cache miss even if
we fail the first hit.

Now, for generic keys & values, that picture changes significantly, and having both keys & values inlined is now quite probably bad
for cache efficiency. Here then, we'd have them separate, which means a minimum of _two_ cache misses per lookup: */

template <typename T>
struct ItemHashtable
{
    u64* keys;
    T* values;

    i32 count;
    i32 capacity;
};

/*
So, for the generic case (and to be able to validate these assumptions through proper testing) we're going to want
to be able to have either keys + values as separate or as interleaved.

The problem is how to do this without having to write the code twice and without affecting performance.
We could somehow templatize an internal structure that dictates the layout, based on the size of K and V types, but then all access
would be through pass-through methods in that structure, which I guess is fine as long as they're inlined?
*/

template <typename K> INLINE u64  DefaultHashFunc( K const& key )           { return Hash64( &key, I32( SIZEOF(K) ) ); }
template <typename K> INLINE bool DefaultEqFunc( K const& a, K const& b )   { return a == b; }

// Some specializations
template <> INLINE u64  DefaultHashFunc< char const* >( char const* const& key )                    { return Hash64( key, StringLength( key ) ); }
template <> INLINE bool DefaultEqFunc< const char* >( char const* const& a, char const* const& b )  { return StringEquals( a, b ); }
template <> INLINE u64  DefaultHashFunc< String >( String const& key )      { return key.Hash(); }


template <typename K>
const K ZeroKey{};

// NOTE Type K must have a default constructor, and all keys must be different to the default-constructed value
template <typename K, typename V, typename AllocType = Allocator>
struct Hashtable
{
    struct Item
    {
        K const& key;
        V& value;
    };

    using HashFunc = u64 (*)( K const& key );
    using KeysEqFunc = bool (*)( K const& a, K const& b );

    enum Flags
    {
        None = 0,
        FixedSize = 0x1,    // For stable pointers to content, must provide expectedSize as appropriate
    };


    K* keys;
    V* values;

    AllocType* allocator;
    HashFunc hashFunc;
    KeysEqFunc eqFunc;
    i32 count;
    i32 capacity;
    u32 flags;


    explicit Hashtable( int expectedSize = 0, AllocType* alloc = CTX_ALLOC, 
                        HashFunc hashFunc_ = DefaultHashFunc<K>, KeysEqFunc eqFunc_ = DefaultEqFunc<K>, u32 flags_ = 0 )
        : keys( nullptr )
        , values( nullptr )
        , allocator( alloc )
        , hashFunc( hashFunc_ )
        , eqFunc( eqFunc_ )
        , count( 0 )
        , capacity( 0 )
        , flags( flags_ )
    {
        ASSERT( expectedSize || !(flags & FixedSize) );
        ASSERT( allocator );

        if( expectedSize )
            Grow( NextPowerOf2( 2 * expectedSize ) );
    }

    // Disallow implicit copying
    Hashtable( const Hashtable& ) = delete;
    Hashtable& operator =( const Hashtable& ) = delete;

    Hashtable( Hashtable&& rhs )
        : allocator( rhs.allocator )
        , flags( rhs.flags )
    {
        *this = std::move( rhs );
    }


    bool Empty() const { return count == 0; }

#if 0
    Hashtable& operator =( Hashtable&& rhs )
    {
        Destroy();

        // FIXME We need to force a copy (or error out) when the allocators differ?
        keys     = rhs.keys;
        value    = rhs.value;
        hashFunc = rhs.hashFunc;
        eqFunc   = rhs.eqFunc;
        count    = rhs.count;
        capacity = rhs.capacity;

        rhs.Destroy();

        return *this;
    }
#else
    Hashtable& operator =( Hashtable&& ) = delete;
#endif

    V* Get( K const& key )
    {
        ASSERT( !eqFunc( key, ZeroKey<K> ) );

        if( count == 0 )
            return nullptr;

        ASSERT( count < capacity );

        u64 hash = hashFunc( key );
        u32 i = hash & (capacity - 1);

        u32 startIdx = i;
        for( ;; )
        {
            if( eqFunc( keys[i], key ) )
                return &values[i];
            else if( eqFunc( keys[i], ZeroKey<K> ) )
                return nullptr;

            i++;
            if( i == U32( capacity ) )
                i = 0;
            if( i == startIdx )
            {
                // Something went horribly wrong
                ASSERT( false );
                return nullptr;
            }
        }
    }

    V* PutEmpty( K const& key, const bool clear = true, bool* occupiedOut = nullptr )
    {
        ASSERT( !eqFunc( key, ZeroKey<K> ) );

        // Super conservative but easy to work with
        if( 2 * count >= capacity )
            Grow( 2 * capacity );
        ASSERT( 2 * count < capacity );

        u64 hash = hashFunc( key );
        u32 i = hash & (capacity - 1);

        u32 startIdx = i;
        for( ;; )
        {
            if( eqFunc( keys[i], key ) )
            {
                if( clear )
                    // TODO Call constructor or clear to zero depending on std::is_trivially_copyable(T)
                    INIT( values[i] )();
                if( occupiedOut )
                    *occupiedOut = true;
                return &values[i];
            }
            else if( eqFunc( keys[i], ZeroKey<K> ) )
            {
                keys[i] = key;
                if( clear )
                    // TODO Call constructor or clear to zero depending on std::is_trivially_copyable(T)
                    INIT( values[i] )();
                ++count;
                if( occupiedOut )
                    *occupiedOut = false;
                return &values[i];
            }

            i++;
            if( i == U32( capacity ) )
                i = 0;
            if( i == startIdx )
            {
                // Something went horribly wrong
                ASSERT( false );
                return nullptr;
            }
        }
    }

    V* Put( K const& key, V const& value )
    {
        V* result = PutEmpty( key, false );
        ASSERT( result );

        *result = value;
        return result;
    }

    V* GetOrPutEmpty( K const& key, bool* foundOut = nullptr )
    {
        bool found;
        V* result = PutEmpty( key, false, &found );
        ASSERT( result );

        if( !found )
            // TODO Call constructor or clear to zero depending on std::is_trivially_copyable(T)
            INIT( *result )();

        if( foundOut )
            *foundOut = found;
        return result;
    }

    V* GetOrPut( K const& key, V const& value, bool* foundOut = nullptr )
    {
        bool found;
        V* result = PutEmpty( key, false, &found );
        ASSERT( result );

        if( !found )
            *result = value;

        if( foundOut )
            *foundOut = found;
        return result;
    }

    // TODO Deleting based on Robid Hood or similar

    template <typename E>
    struct BaseIterator
    {
        BaseIterator( Hashtable const& table_ )
            : table( table_ )
        {
            current = table.keys - 1;
            Next();
        }
        virtual ~BaseIterator() {}

        explicit operator bool() const { return current != nullptr; }

        virtual E Get() const = 0;
        E operator * () const { return Get(); }

        BaseIterator& operator ++()
        {
            Next();
            return *this;
        }

    protected:
        Hashtable const& table;
        K* current;

    private:
        void Next()
        {
            do
            {
                current++;
            }
            while( current < table.keys + table.capacity && 
                table.eqFunc( *current, ZeroKey<K> ) );

            if( current >= table.keys + table.capacity )
                current = nullptr;
        }
    };

    struct ItemIterator : public BaseIterator<Item>
    {
        ItemIterator( Hashtable const& table_ )
            : BaseIterator<Item>( table_ )
        {}

        Item Get() const override
        {
            ASSERT( current );
            V& currentValue = table.values[ current - table.keys ];
            Hashtable::Item result = { *current, currentValue };
            return result;
        }
    };

    struct KeyIterator : public BaseIterator<K const&>
    {
        KeyIterator( Hashtable const& table_ )
            : BaseIterator<K const&>( table_ )
        {}

        K const& Get() const override
        {
            ASSERT( current );
            return *current;
        }
    };

    struct ValueIterator : public BaseIterator<V&>
    {
        ValueIterator( Hashtable const& table_ )
            : BaseIterator<V&>( table_ )
        {}

        V& Get() const override
        {
            ASSERT( current );
            V& currentValue = table.values[ current - table.keys ];
            return currentValue;
        }
    };

    struct ConstValueIterator : public BaseIterator<V const&>
    {
        ConstValueIterator( Hashtable const& table_ )
            : BaseIterator<V const&>( table_ )
        {}

        V const& Get() const override
        {
            ASSERT( current );
            V& currentValue = table.values[ current - table.keys ];
            return currentValue;
        }
    };

    ItemIterator Items() { return ItemIterator( *this ); }
    KeyIterator Keys() const { return KeyIterator( *this ); }
    ValueIterator Values() { return ValueIterator( *this ); }
    ConstValueIterator ConstValues() const { return ConstValueIterator( *this ); }

private:
    void Grow( int newCapacity )
    {
        ASSERT( !(flags & FixedSize) || !capacity );

        newCapacity = Max( newCapacity, 16 );
        ASSERT( IsPowerOf2( newCapacity ) );

        int oldCapacity = capacity;
        K* oldKeys = keys;
        V* oldValues = values;

        count = 0;
        capacity = newCapacity;

        // TODO Check generated code for instances with different allocator types is still being inlined
        void* newMemory = ALLOC( allocator, capacity * (SIZEOF(K) + SIZEOF(V)), Memory::NoClear() );
        keys   = (K*)newMemory;
        values = (V*)((u8*)newMemory + capacity * SIZEOF(K));

        for( int i = 0; i < capacity; ++i )
            // TODO Call constructor or clear to zero depending on std::is_trivially_copyable(T)
            INIT( keys[i] )();
        for( int i = 0; i < oldCapacity; ++i )
        {
            if( !eqFunc( oldKeys[i], ZeroKey<K> ) )
                Put( oldKeys[i], oldValues[i] );
        }

        FREE( allocator, oldKeys ); // Handles both
    }
};


/////     RING BUFFER    /////
// Circular buffer backed by an array with a stable maximum size (no allocations after init).
// Default behaviour is similar to a FIFO queue where new items are Push()ed onto a virtual "head" cursor,
// while Pop() returns the item at the "tail" (oldest). Can also remove newest item with PopHead() for a LIFO stack behaviour.
// Head and tail will wrap around when needed and can never overlap.
// Can be iterated both from tail to head (oldest to newest) or the other way around.

// FIXME 'Head' & 'Tail' here have the exact opposite meaning you would expect for a queue, hence are super confusing!
// TODO Look at the code inside #ifdef MEM_REPLACE_PLACEHOLDER in https://github.com/cmuratori/refterm/blob/main/refterm_example_source_buffer.c
// for an example of how to speed up linear reads & writes that go beyond the end of the buffer,
// by just mapping the same block of memory twice back to back!

template <typename T, typename AllocType = Allocator>
struct RingBuffer
{
    Array<T, AllocType> buffer;
    i32 onePastHeadIndex;       // Points to next slot to write
    i32 count;


    RingBuffer()
    {}

    RingBuffer( i32 capacity, AllocType* allocator = CTX_ALLOC, MemoryParams params = Memory::NoClear() )
        : buffer( capacity, allocator, params )
        , onePastHeadIndex( 0 )
        , count( 0 )
    {
        ASSERT( IsPowerOf2( capacity ) );
        buffer.ResizeToCapacity();
    }

    int Count() const { return count; }
    int Capacity() const { return buffer.capacity; }
    bool Available() const { return count < buffer.capacity; }

    void Clear()
    {
        onePastHeadIndex = 0;
        count = 0;
    }

    void ClearToZero()
    {
        Clear();
        PZERO( buffer.data, buffer.count * sizeof(T) );
    }

    T* PushEmpty( bool clear = true )
    {
        T* result = buffer.data + onePastHeadIndex;
        onePastHeadIndex = (onePastHeadIndex + 1) & (buffer.capacity - 1);
        if( Available() )
            count++;

        if( clear )
            ZEROP( result, sizeof(T) );
        return result;
    }

    T* Push( const T& item )
    {
        T* result = PushEmpty( false );
        *result = item;
        return result;
    }

    T Pop()
    {
        ASSERT( count > 0 );
        int tailIndex = TailIndex();
        count--;

        return buffer.data[tailIndex];
    }
    bool TryPop( T* out )
    {
        bool canPop = count > 0; 
        if( canPop )
            *out = Pop();

        return canPop;
    }

    T PopHead()
    {
        ASSERT( count > 0 );
        onePastHeadIndex = (onePastHeadIndex - 1) & (buffer.capacity - 1);
        count--;

        return buffer.data[onePastHeadIndex];
    }
    bool TryPopHead( T* out )
    {
        bool canPop = count > 0; 
        if( canPop )
            *out = PopHead();

        return canPop;
    }

    bool Contains( const T& item ) const
    {
        return buffer.Contains( item );
    }

    T& Head( int offset = 0 )
    {
        ASSERT( offset >= 0, "Only positive offsets" );
        ASSERT( count > offset, "Not enough items in buffer" );
        int index = IndexFromHeadOffset( -offset );
        return buffer.data[index];
    }
    T const& Head( int offset = 0 ) const
    {
        return ((RingBuffer<T>*)this)->Head( offset );
    }

    T& Tail( int offset = 0 )
    {
        ASSERT( offset >= 0, "Only positive offsets" );
        ASSERT( count > offset, "Not enough items in buffer" );
        int head_offset = count - 1 - offset;
        int idx = IndexFromHeadOffset( -head_offset );
        return buffer.data[idx];
    }
    T const& Tail( int offset = 0 ) const
    {
        return ((RingBuffer<T>*)this)->Tail( offset );
    }

    int TailIndex() const
    {
        return (int)(&Tail() - buffer.data);
    }


    struct Iterator
    {
    protected:
        RingBuffer<T>& b;
        T* current;
        i32 currentIndex;
        bool forward;

    public:
        Iterator( RingBuffer& buffer_, bool forward_ = true )
            : b( buffer_ )
            , current( nullptr )
            , currentIndex( 0 )
            , forward( forward_ )
        {
            if( b.Count() )
            {
                currentIndex = forward ? b.TailIndex() : b.IndexFromHeadOffset( 0 );
                current = &b.buffer.data[currentIndex];
            }
        }

        operator  bool() { return current != nullptr; }
        T& operator * () { return *current; }
        T* operator ->() { return current; }

        // Prefix
        inline Iterator& operator ++()
        {
            if( current )
            {
                if( forward )
                {
                    currentIndex = (currentIndex + 1) & (b.buffer.capacity - 1);
                    current = (currentIndex == b.onePastHeadIndex) ? nullptr : &b.buffer.data[currentIndex];
                }
                else
                {
                    int tailIndex = b.TailIndex();

                    currentIndex--;
                    bool wrapped = b.onePastHeadIndex < tailIndex;
                    bool finished = wrapped ? (currentIndex >= b.onePastHeadIndex && currentIndex < tailIndex)
                                            : (currentIndex < tailIndex);

                    currentIndex &= (b.buffer.capacity - 1);
                    current = finished ? nullptr : &b.buffer.data[currentIndex];
                }
            }

            return *this;
        }
    };
    Iterator MakeIterator( bool forward = true )
    {
        return Iterator( *this, forward );
    }

private:
    int IndexFromHeadOffset( int offset ) const
    {
        int result = onePastHeadIndex + offset - 1;
        result &= (buffer.capacity - 1);

        return result;
    }
};


// A Version of RingBuffer that is thread-safe and lock-free (multi-producer, multi-consumer).
// Any operations that would require locking the full buffer have been removed.
// Can be used as a fixed-capacity thread-safe queue.

// FIXME This type is very broken! Review all ops agains!
template <typename T, typename AllocType = Allocator>
struct SyncRingBuffer
{
    Array<T, AllocType> buffer;
    // 32 bits for 'count' | 32 bits for 'onePastHeadIndex'
    atomic_u64 indexData;


    SyncRingBuffer()
    {
        _CheckLayout();
    }

    SyncRingBuffer( i32 capacity, AllocType* allocator = CTX_ALLOC, MemoryParams params = Memory::NoClear() )
        : buffer( capacity, allocator, params )
        , indexData( 0 )
    {
        _CheckLayout();
        ASSERT( IsPowerOf2( capacity ) );
        buffer.ResizeToCapacity();
    }

    void _CheckLayout()
    {
        ASSERT( indexData.is_lock_free(), "'indexData' attribute is not lock-free (check alignment?)" );
    };

    int Count() const { return (int)(indexData.LOAD_ACQUIRE() >> 32); }
    int Capacity() const { return buffer.capacity; }
    bool Empty() const { return Count() > 0; }
    bool Available() const { return Count() < buffer.capacity; }

    void Clear()
    {
        indexData.STORE_RELEASE( 0 );
    }

    T* PushEmpty( bool clear = true )
    {
        u32 headIndex;
        u64 newData, curData = indexData.LOAD_RELAXED();
        do
        {
            headIndex = curData & 0xFFFFFFFF;
            u64 count = curData >> 32;

            u64 onePastHeadIndex = (headIndex + 1) & (buffer.capacity - 1);
            if( count < (u64)buffer.capacity )
                count++;

            newData = (count << 32) | onePastHeadIndex;
        } while( !indexData.COMPARE_EXCHANGE_ACQREL( curData, newData ) );

        T* result = buffer.data + headIndex;
        // FIXME This is wrong. Consumer can observe result pointer not cleared
        if( clear )
            ZEROP( result, sizeof(T) );

        return result;
    }

    T* Push( const T& item )
    {
        T* result = PushEmpty( false );
        // FIXME Consumer can observe result pointer being empty
        *result = item;
        return result;
    }

    // TODO Remove
    T* PushEmptyRangeAdjacent( int rangeLen, bool clear = true )
    {
        u32 len = U32(rangeLen);

        u32 headIndex;
        u64 newData, curData = indexData.LOAD_RELAXED();
        do
        {
            headIndex = curData & 0xFFFFFFFF;
            u64 count = curData >> 32;

            u32 remaining = buffer.capacity - headIndex;
            bool adjacent = remaining >= len;

            // If we're close to the end of the buffer and the range doesn't fit, skip the remaining bytes and place it at the front
            u64 onePastHeadIndex = adjacent ? (headIndex + len) & (buffer.capacity - 1) : len;
            if( !adjacent )
                headIndex = 0;
            count += (adjacent ? len : remaining + len);
            count = Min( count, (u64)buffer.capacity );

            newData = (count << 32) | onePastHeadIndex;
        } while( !indexData.COMPARE_EXCHANGE_ACQREL( curData, newData ) );

        T* result = buffer.data + headIndex;
        if( clear )
            // TODO Clear the skipped end of the buffer too?
            ZEROP( result, rangeLen * SIZEOF(T) );

        return result;
    }

    // TODO Remove
    T* PushRangeAdjacent( T const* item, int rangeLen )
    {
        T* result = PushEmptyRangeAdjacent( rangeLen, false );
        COPYP( item, result, rangeLen * sizeof(T) );
        return result;
    }

    bool TryPop( T* out )
    {
        u64 count, onePastHeadIndex;
        u64 newData, curData = indexData.LOAD_ACQUIRE();
        do
        {
            onePastHeadIndex = curData & 0xFFFFFFFF;
            count = curData >> 32;

            // Try to do the whole #! and just return whether we did anything at the end
            u64 newCount = (count > 0) ? count - 1 : count;
            newData = (newCount << 32) | onePastHeadIndex;

        } while( !indexData.COMPARE_EXCHANGE_ACQREL( curData, newData ) );

        bool success = count > 0;
        if( success )
        {
            u64 tailIndex = onePastHeadIndex - count;
            tailIndex &= (buffer.capacity - 1);
            // FIXME By the time we do the copy someone could have pushed at this index
            *out = buffer.data[tailIndex];
        }
        return success;
    }

    bool TryPopHead( T* out )
    {
        u64 count, onePastHeadIndex;
        u64 newData, curData = indexData.LOAD_ACQUIRE();
        do
        {
            onePastHeadIndex = curData & 0xFFFFFFFF;
            count = curData >> 32;

            // Try to do the whole #! and just return whether we did anything at the end
            onePastHeadIndex = count > 0 ? ((onePastHeadIndex - 1) & (buffer.capacity - 1)) : onePastHeadIndex;
            u64 newCount     = count > 0 ? (count - 1) : count;
            newData = (newCount << 32) | onePastHeadIndex;

        } while( !indexData.COMPARE_EXCHANGE_ACQREL( curData, newData ) );

        bool success = count > 0;
        if( success )
            // FIXME By the time we do the copy someone could have pushed again
            *out = buffer.data[onePastHeadIndex];

        return success;
    }

    // TODO PeekHead & PeekTail

    T& Head( int offset = 0 )
    {
        ASSERT( offset >= 0, "Only positive offsets" );
        int count = 0;
        int index = IndexFromHeadOffset( -offset, &count );
        ASSERT( count > offset, "Not enough items in buffer" );

        return buffer.data[index];
    }
    T const& Head( int offset = 0 ) const
    {
        return ((SyncRingBuffer<T>*)this)->Head( offset );
    }

    T& Tail( int offset = 0 )
    {
        ASSERT( offset >= 0, "Only positive offsets" );
        int count = 0;
        int idx = IndexFromTailOffset( offset, &count );
        ASSERT( count > offset, "Not enough items in buffer" );

        return buffer.data[idx];
    }
    T const& Tail( int offset = 0 ) const
    {
        return ((SyncRingBuffer<T>*)this)->Tail( offset );
    }

private:
    int TailIndex() const
    {
        return IndexFromTailOffset( 0, nullptr );
    }

    int IndexFromHeadOffset( int offset, int* countOut = nullptr ) const
    {
        u64 curData = indexData.LOAD_ACQUIRE();
        u32 onePastHeadIndex = curData & 0xFFFFFFFF;
        if( countOut )
            *countOut = (int)(curData >> 32);

        int result = (int)(onePastHeadIndex + offset - 1);
        result &= (buffer.capacity - 1);

        return result;
    }

    int IndexFromTailOffset( int offset, int* countOut = nullptr ) const
    {
        u64 curData = indexData.LOAD_ACQUIRE();
        u32 onePastHeadIndex = curData & 0xFFFFFFFF;
        u32 count = curData >> 32;

        int result = (int)(onePastHeadIndex - count + offset);
        result &= (buffer.capacity - 1);

        if( countOut )
            *countOut = (int)count;
        return result;
    }


public:
    // NOTE Consider this just an "imprecise view" of the buffer contents
    // NOTE This is inherently NOT threadsafe, hence made read-only
    struct Iterator
    {
    protected:
        SyncRingBuffer<T>& b;
        T const* current;
        i32 currentIndex;
        bool forward;

    public:
        Iterator( SyncRingBuffer& buffer_, bool forward_ = true )
            : b( buffer_ )
                , current( nullptr )
                , currentIndex( 0 )
                , forward( forward_ )
        {
            if( b.Count() )
            {
                currentIndex = forward ? b.TailIndex() : b.IndexFromHeadOffset( 0 );
                current = &b.buffer.data[currentIndex];
            }
        }

        operator bool() { return current != nullptr; }
        T const& operator * () { return *current; }
        T const* operator ->() { return current; }

        // Prefix
        inline Iterator& operator ++()
        {
            if( current )
            {
                int onePastHeadIndex = b.IndexFromHeadOffset( 1 );
                int tailIndex = b.TailIndex();

                if( forward )
                {
                    currentIndex = (currentIndex + 1) & (b.buffer.capacity - 1);
                    current = (currentIndex == onePastHeadIndex) ? nullptr : &b.buffer.data[currentIndex];
                }
                else
                {
                    currentIndex--;
                    bool wrapped = onePastHeadIndex < tailIndex;
                    bool finished = wrapped ? (currentIndex >= onePastHeadIndex && currentIndex < tailIndex)
                                            : (currentIndex < tailIndex);

                    currentIndex &= (b.buffer.capacity - 1);
                    current = finished ? nullptr : &b.buffer.data[currentIndex];
                }
            }

            return *this;
        }
    };
    Iterator MakeIterator( bool forward = true )
    {
        return Iterator( *this, forward );
    }
};



/////     SYNC QUEUE    /////
// Thread safe queue.
// TODO Lock-free
// https://moodycamel.com/blog/2013/a-fast-lock-free-queue-for-c++.htm
// https://moodycamel.com/blog/2014/a-fast-general-purpose-lock-free-queue-for-c++.htm

// TODO Consider how to avoid false-sharing, given that a producer thread will always access the tail and a consumer will access the head
// TODO Also consider switching all datatypes of this kind to a 'virtual stream' model .. https://fgiesen.wordpress.com/2010/12/14/ring-buffers-and-queues/
template <typename T, typename AllocType = Allocator>
struct SyncQueue
{
    struct Page
    {
        Page* next;
        i32 capacity;
        i32 count;
        i32 onePastTailIndex;

        int HeadIndex() const
        {
            int result = onePastTailIndex - count;
            result &= (capacity - 1 );
            return result;
        }
    };

    Mutex mutex;
    AllocType* allocator;

    Page* head;
    Page* tail;
    Page* firstFree;
    i32 count;

    SyncQueue()
        : allocator( nullptr )
        , head( nullptr )
        , tail( nullptr )
        , firstFree( nullptr )
        , count( 0 )
    {}

    SyncQueue( int pageCapacity, AllocType* alloc = CTX_ALLOC )
        : allocator( alloc )
        , head( nullptr )
        , tail( nullptr )
        , firstFree( nullptr )
        , count( 0 )
    {
        ASSERT( IsPowerOf2( pageCapacity ) );
        head = tail = AllocPage( pageCapacity );
    }

private:
    T* PageItemUnsafe( Page* p, int index )
    {
        // Skip Page header
        return (T*)((u8*)p + sizeof(Page) + index * sizeof(T));
    }

    T* PushEmptyUnsafe( bool clear = true )
    {
        if( tail->count >= tail->capacity )
        {
            tail->next = AllocPage( tail->capacity );
            tail = tail->next;
        }

        T* result = PageItemUnsafe( tail, tail->onePastTailIndex );
        tail->onePastTailIndex = (tail->onePastTailIndex + 1) & (tail->capacity - 1);
        tail->count++;

        count++;

        if( clear )
            ZEROP( result, sizeof(T) );
        return result;
    }

public:
    T* PushEmpty( bool clear = true )
    {
        Mutex::Scope lock( mutex );
        return PushEmptyUnsafe( clear );
    }

    T* Push( const T& item )
    {
        Mutex::Scope lock( mutex );
        T* result = PushEmptyUnsafe( false );
        *result = item;

        return result;
    }

    T* Push( T&& item )
    {
        Mutex::Scope lock( mutex );
        T* result = PushEmptyUnsafe( false );
        *result = std::move( item );

        return result;
    }

    bool TryPop( T* out )
    {
        bool canPop = count > 0;
        if( canPop )
        {
            Mutex::Scope lock( mutex );

            canPop = count > 0;
            if( canPop )
            {
                T* result = PageItemUnsafe( head, head->HeadIndex() );
                *out = std::move( *result );
                head->count--;

                if( head->count == 0 && head != tail )
                {
                    Page* oldHead = head;
                    head = head->next;

                    oldHead->next = firstFree;
                    firstFree = oldHead;
                }

                count--;
            }
        }

        return canPop;
    }

    T* Find( T const& item )
    {
        Mutex::Scope lock( mutex );

        Page* p = head;
        while( p )
        {
            for( int i = 0; i < p->count; ++i )
            {
                int index = (p->HeadIndex() + i) & (p->capacity - 1);
                T* it = PageItemUnsafe( p, index );
                if( *it == item )
                    return it;
            }
            p = p->next;
        }

        return nullptr;
    }

    T const* Find( T const& item ) const
    {
        return ((SyncQueue<T, AllocType>*)this)->Find( item );
    }

    bool Contains( T const& item ) const
    {
        return Find( item ) != nullptr;
    }

    template<typename Predicate>
    T* Find( Predicate&& pred )
    {
        Mutex::Scope lock( mutex );

        Page* p = head;
        while( p )
        {
            for( int i = 0; i < p->count; ++i )
            {
                int index = (p->HeadIndex() + i) & (p->capacity - 1);
                T* it = PageItemUnsafe( p, index );
                if( pred( *(T const*)it ) )
                    return it;
            }
            p = p->next;
        }

        return nullptr;
    }

    template<typename Predicate>
    T const* Find( Predicate&& pred ) const
    {
        return ((SyncQueue<T, AllocType>*)this)->Find( pred );
    }

    template<typename Predicate>
    bool Contains( Predicate&& pred ) const
    {
        return Find( pred ) != nullptr;
    }

private:

    Page* AllocPage( int pageCapacity )
    {
        Page* result = nullptr;

        if( firstFree )
        {
            result = firstFree;
            firstFree = firstFree->next;
        }
        else
        {
            result = (Page*)ALLOC( allocator, Sz(sizeof(Page) + pageCapacity * sizeof(T)) );
        }

        *result = {};
        result->capacity = pageCapacity;

        return result;
    }
};
