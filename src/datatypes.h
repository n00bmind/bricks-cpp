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
    {}

    // NOTE All newly allocated arrays start empty
    // NOTE You cannot specify an AllocType for the instance and then not provide an allocator
    explicit Array( i32 capacity_, AllocType* allocator_ = CTX_ALLOC, MemoryParams params = {} )
        : count( 0 )
        , capacity( capacity_ )
        , allocator( allocator_ )
        , memParams( params )
    {
        ASSERT( capacity > 0 );
        ASSERT( allocator );
        data = ALLOC_ARRAY( allocator, T, capacity, memParams );
    }

    Array( T* buffer, i32 count_, i32 capacity_ )
        : data( buffer )
        , count( count_ )
        , capacity( capacity_ )
    {
        ASSERT( data && count >= 0 && capacity > 0 && count <= capacity );
    }

    // NOTE Arrays initialized from a buffer have count set equal to their capacity by default
    Array( T* buffer, i32 bufferLen )
        : Array( buffer, bufferLen, bufferLen )
    {}

    ~Array()
    {
        if( allocator )
            FREE( allocator, data, memParams );
    }

    operator Buffer<T>()
    {
        return Buffer<T>( data, count );
    }


    T*          begin()         { return data; }
    const T*    begin() const   { return data; }
    T*          end()           { return data + count; }
    const T*    end() const     { return data + count; }

    T&          First()         { ASSERT( count > 0 ); return data[0]; }
    T const&    First() const   { ASSERT( count > 0 ); return data[0]; }
    T&          Last()          { ASSERT( count > 0 ); return data[count - 1]; }
    T const&    Last() const    { ASSERT( count > 0 ); return data[count - 1]; }

    bool        Empty() const   { return count == 0; }

    template <typename AllocType2 = Allocator>
    bool operator ==( Array<T, AllocType2> const& other ) const
    {
        return count == other.count && PEQUAL( data, other.data, count * SIZEOF(T) );
    }

    explicit operator bool() const
    {
        return data != nullptr && count != 0;
    }

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
    T* PushEmpty( bool clear = false )
    {
        ASSERT( count < capacity );
        T* result = data + count++;
        if( clear )
            INIT( *result );

        return result;
    }

    T* Push( const T& item )
    {
        T* slot = PushEmpty( false );
        INIT( *slot )( item );

        return slot;
    }

    void Remove( T* item )
    {
        ASSERT( item >= begin() && item < end() );

        T* last = &Last();
        if( item != last )
            *item = *last;

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
    T* Find( Predicate p )
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
    T const* Find( Predicate p ) const
    {
        T* slot = ((Array<T, AllocType>*)this)->Find( p );
        return slot;
    }

    template <class Predicate>
    bool Contains( Predicate p ) const
    {
        T const* slot = Find( p );
        return slot != nullptr;
    }

    void Append( T const* buffer, int bufferLen )
    {
        ASSERT( Available() >= bufferLen );

        for( int i = 0; i < bufferLen; ++i )
            Push( buffer[i] );
    }

    template <typename AllocType2 = Allocator>
    void Append( Array<T, AllocType2> const& array )
    {
        Append( array.data, array.count );
    }

    // Deep copy
    template <typename AllocType2 = Allocator>
    Array<T, AllocType2> Clone( AllocType2* allocator = nullptr ) const
    {
        Array<T, AllocType2> result( count, allocator ? allocator : CTX_ALLOC );
        result.ResizeToCapacity();
        PCOPY( data, result.data, count * SIZEOF(T) );
        return result;
    }

    template <typename AllocType2 = Allocator>
    static Array<T, AllocType2> Clone( Buffer<T> buffer, AllocType2* allocator = nullptr )
    {
        Array<T, AllocType2> result( I32( buffer.length ), allocator ? allocator : CTX_ALLOC );
        result.ResizeToCapacity();
        PCOPY( buffer.data, result.data, buffer.length * SIZEOF(T) );
        return result;
    }

    template <typename AllocType2 = Allocator>
    void CopyTo( Array<T, AllocType2>* out ) const
    {
        ASSERT( out->capacity >= count );
        PCOPY( data, out->data, count * SIZEOF(T) );
        out->count = count;
    }

    void CopyTo( T* buffer ) const
    {
        PCOPY( data, buffer, count * SIZEOF(T) );
    }

    void CopyFrom( const T* buffer, int count_ )
    {
        ASSERT( count_ > 0 && capacity >= count_ );
        PCOPY( buffer, data, count_ * SIZEOF(T) );
    }
};

#define ARRAY( T, len, name ) \
        T UNIQUE(__array_storage)[len] = {}; \
        Array<T> name = { UNIQUE(__array_storage), 0, len };

#if 0
template <typename T, typename AllocType>
const Array<T, AllocType> Array<T, AllocType>::Empty = {};
#endif


/////     BUCKET ARRAY     /////

// TODO There's two conflicting views on this datatype rn:
// - One would be that we want to keep it fast (both to iterate and to compact into an array), so we want to keep buckets compacted
//   at all times, where any item removed is swapped with the last one (or subsequent items are moved down) so that we can iterate
//   each bucket fast with just the pointer to the start of the bucket and its count.
// - The second view is that this is a datatype used mainly for storage, and so we want items to keep a stable address, and be able
//   to return an Idx/Handle to them. For that we'd need to keep items in place always, and keep something like an occupancy bitfield.
//
// Now, Part Pools in KoM have this same exact duality, and I think the concept of a roster used there can help, but used in the
// opposite sense it's currently being used there.. Instead of the roster being a way to iterate consecutively even in the presence
// of gaps in the pool, we'd much rather have the buckets be consecutive always (option 1) and use the roster as the thing that
// a Handle points to (for option 2).
//
// The smart thing to do is probably separate these two objectives in two separate datatypes: BucketArray tries to be as simple as
// possible and just compacts its items down inside the buckets, while PagedPool is composed of a BucketArray plus a roster that it
// uses to hand out Handles to any added items (complete with generation numbers etc.)

template <typename T, typename AllocType = Allocator>
struct BucketArray
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

        Bucket( i32 capacity_, AllocType* allocator_, MemoryParams memParams_ )
            : next( nullptr )
            , prev( nullptr )
            , allocator( allocator_ )
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
    static const BucketArray<T> Empty;
#endif

    BucketArray()
        : last( nullptr )
        , firstFree( nullptr )
        , allocator( nullptr )
        , memParams{}
        , count( 0 )
    {}

    explicit BucketArray( i32 bucketSize, AllocType* allocator_ = CTX_ALLOC, MemoryParams params = {} )
        : first( bucketSize, allocator_, params )
        , last( &first )
        , firstFree( nullptr )
        , allocator( allocator_ )
        , memParams( params )
        , count( 0 )
    {
        ASSERT( allocator );
    }

    BucketArray( BucketArray<T>&& other )
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

    ~BucketArray()
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
    BucketArray( const BucketArray& ) = delete;
    BucketArray& operator =( const BucketArray& ) = delete;

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
            INIT( *result );

        return result;
    }

    T* Push( const T& item )
    {
        T* slot = PushEmpty( false );
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
        return const_cast<BucketArray const*>(this)->At( index );
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
        T* result = ((BucketArray<T>*)this)->Find( item, cmp );
        return result;
    }

    bool Contains( T const& item ) const
    {
        T const* result = Find( item, cmp );
        return result != nullptr;
    }

    template <class Predicate>
    T* Find( Predicate p )
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
    T const* Find( Predicate p ) const
    {
        T* slot = ((BucketArray<T>*)this)->Find( p );
        return slot;
    }

    template <class Predicate>
    bool Contains( Predicate p ) const
    {
        T const* slot = Find( p );
        return slot != nullptr;
    }


    void Append( T const* buffer, int bufferLen )
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
        Append( array.data, array.count );
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
    Array<T, AllocType2> ToArray( AllocType2* allocator_ = nullptr ) const
    {
        Array<T, AllocType2> result( count, allocator_ ? allocator_ : CTX_ALLOC );
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
const BucketArray<T, AllocType> BucketArray<T, AllocType>::Empty = {};
#endif


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

template <typename K> INLINE u64  DefaultHashFunc( K const& key )          { return Hash64( &key, I32( SIZEOF(K) ) ); }
template <typename K> INLINE bool DefaultEqFunc( K const& a, K const& b )  { return a == b; }

template <typename K>
const K ZeroKey = K();

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
    MemoryParams memParams;
    u32 flags;


    explicit Hashtable( int expectedSize = 0, AllocType* allocator_ = CTX_ALLOC, 
                        HashFunc hashFunc_ = DefaultHashFunc<K>, KeysEqFunc eqFunc_ = DefaultEqFunc<K>, u32 flags_ = 0 )
        : keys( nullptr )
        , values( nullptr )
        , allocator( allocator_ )
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
        , memParams( rhs.memParams )
        , flags( rhs.flags )
    {
        *this = std::move( rhs );
    }

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
                // Something went horribly wrong
                return nullptr;
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
            if( eqFunc( keys[i], ZeroKey<K> ) )
            {
                keys[i] = key;
                if( clear )
                    INIT( values[i] )();
                ++count;
                if( occupiedOut )
                    *occupiedOut = false;
                return &values[i];
            }
            else if( eqFunc( keys[i], key ) )
            {
                if( clear )
                    INIT( values[i] )();
                if( occupiedOut )
                    *occupiedOut = true;
                return &values[i];
            }

            i++;
            if( i == U32( capacity ) )
                i = 0;
            if( i == startIdx )
                // Something went horribly wrong
                return nullptr;
        }
    }

    V* Put( K const& key, V const& value )
    {
        V* result = PutEmpty( key, false );
        ASSERT( result );

        *result = value;
        return result;
    }

    V* PutIfNotFound( K const& key, V const& value )
    {
        bool found;
        V* result = PutEmpty( key, false, &found );
        ASSERT( result );

        if( !found )
            *result = value;

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
            : BaseIterator( table_ )
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
            : BaseIterator( table_ )
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
            : BaseIterator( table_ )
        {}

        V& Get() const override
        {
            ASSERT( current );
            V& currentValue = table.values[ current - table.keys ];
            return currentValue;
        }
    };

    ItemIterator Items() { return ItemIterator( *this ); }
    KeyIterator Keys() { return KeyIterator( *this ); }
    ValueIterator Values() { return ValueIterator( *this ); }

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
        void* newMemory = ALLOC( allocator, capacity * (SIZEOF(K) + SIZEOF(V)), { Memory::NoClear } );
        keys   = (K*)newMemory;
        values = (V*)((u8*)newMemory + capacity * SIZEOF(K));

        for( int i = 0; i < capacity; ++i )
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

    RingBuffer( i32 capacity, AllocType* allocator = CTX_ALLOC, MemoryParams params = {} )
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
            , forward( forward_ )
        {
            currentIndex = forward ? b.TailIndex() : b.IndexFromHeadOffset( 0 );
            current = b.Count() ? &b.buffer.data[currentIndex] : nullptr;
        }

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
                    currentIndex = (currentIndex - 1) & (b.buffer.capacity - 1);
                    current = (currentIndex < b.TailIndex()) ? nullptr : &b.buffer.data[currentIndex];
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
    int IndexFromHeadOffset( int offset )
    {
        int result = onePastHeadIndex + offset - 1;
        result &= (buffer.capacity - 1);

        return result;
    }
};


// A Version of RingBuffer that is thread-safe and lock-free
// Any operations that would require locking the full buffer have been removed
// Can be used as a fixed-capacity thread-safe queue

// TODO Make a separate Queue struct that can chain several of this (thread-safely) to create a generic growable queue.

template <typename T, typename AllocType = Allocator>
struct SyncRingBuffer
{
    Array<T, AllocType> buffer;
    union
    {
#if 0
        struct
        {
            i32 _onePastHeadIndex;
            i32 _count;
        };
#endif
        atomic_i64 indexData;
    };


    SyncRingBuffer()
    {
        _CheckLayout();
    }

    SyncRingBuffer( i32 capacity, AllocType* allocator = CTX_ALLOC, MemoryParams params = {} )
        : buffer( capacity, allocator, params )
        , indexData( 0 )
    {
        _CheckLayout();
        ASSERT( IsPowerOf2( capacity ) );
        buffer.ResizeToCapacity();
    }

    void _CheckLayout()
    {
#if 0
        static_assert( offsetof(SyncRingBuffer, _onePastHeadIndex) == offsetof(SyncRingBuffer, indexData), "Bad layout" );
        static_assert( offsetof(SyncRingBuffer, _count) == offsetof(SyncRingBuffer, _onePastHeadIndex) + 4, "Bad layout" );
#endif
        ASSERT( indexData.is_lock_free(), "'indexData' attribute is not lock-free (check alignment?)" );
    };

    int Count() const { return (indexData.LOAD_ACQUIRE() >> 32); }
    int Capacity() const { return buffer.capacity; }
    bool Available() const { return Count() < buffer.capacity; }

    void Clear()
    {
        indexData.STORE_RELEASE( 0 );
    }

    T* PushEmpty( bool clear = true )
    {
        i32 headIndex;
        // TODO Given that we already do an acquire in the CAS below this can probably be relaxed?
        // Does this affect our chances of looping back in any way?
        i64 newData, curData = indexData.LOAD_ACQUIRE();
        do
        {
            headIndex = curData & 0xFFFFFFFF;
            i64 count = curData >> 32;

            i64 onePastHeadIndex = (headIndex + 1) & (buffer.capacity - 1);
            if( count < buffer.capacity )
                count++;

            newData = (count << 32) | onePastHeadIndex;
        } while( !indexData.COMPARE_EXCHANGE_ACQREL( curData, newData ) );

        T* result = buffer.data + headIndex;
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
        i64 count, onePastHeadIndex;
        i64 newData, curData = indexData.LOAD_ACQUIRE();
        do
        {
            onePastHeadIndex = curData & 0xFFFFFFFF;
            count = curData >> 32;

            if( count > 0 )
                count--;
            else
                INVALID_CODE_PATH;

            newData = (count << 32) | onePastHeadIndex;
        } while( !indexData.COMPARE_EXCHANGE_ACQREL( curData, newData ) );

        // count was already decremented so account for that
        i64 tailIndex = onePastHeadIndex - count - 1;
        tailIndex &= (buffer.capacity - 1);

        return buffer.data[tailIndex];
    }

    bool TryPop( T* out )
    {
        i64 count, onePastHeadIndex;
        i64 newData, curData = indexData.LOAD_ACQUIRE();
        do
        {
            onePastHeadIndex = curData & 0xFFFFFFFF;
            count = curData >> 32;

            // Try to do the whole #! and just return whether we did anything at the end
            int newCount = (count > 0) ? count - 1 : count;
            newData = (newCount << 32) | onePastHeadIndex;

        } while( !indexData.COMPARE_EXCHANGE_ACQREL( curData, newData ) );

        bool success = count > 0;
        if( success )
        {
            // count was _not_ decremented this time
            i64 tailIndex = onePastHeadIndex - count;
            tailIndex &= (buffer.capacity - 1);
            *out = buffer.data[tailIndex];
        }
        return success;
    }

    T PopHead()
    {
        i64 count, onePastHeadIndex;
        i64 newData, curData = indexData.LOAD_ACQUIRE();
        do
        {
            onePastHeadIndex = curData & 0xFFFFFFFF;
            count = curData >> 32;

            onePastHeadIndex = (onePastHeadIndex - 1) & (buffer.capacity - 1);
            if( count > 0 )
                count--;
            else
                INVALID_CODE_PATH;

            newData = (count << 32) | onePastHeadIndex;
        } while( !indexData.COMPARE_EXCHANGE_ACQREL( curData, newData ) );

        return buffer.data[onePastHeadIndex];
    }

    bool TryPopHead( T* out )
    {
        i64 count, onePastHeadIndex;
        i64 newData, curData = indexData.LOAD_ACQUIRE();
        do
        {
            onePastHeadIndex = curData & 0xFFFFFFFF;
            count = curData >> 32;

            // Try to do the whole #! and just return whether we did anything at the end
            onePastHeadIndex = count > 0 ? ((onePastHeadIndex - 1) & (buffer.capacity - 1)) : onePastHeadIndex;
            int newCount     = count > 0 ? (count - 1) : count;
            newData = (newCount << 32) | onePastHeadIndex;

        } while( !indexData.COMPARE_EXCHANGE_ACQREL( curData, newData ) );

        bool success = count > 0;
        if( success )
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

    int TailIndex() const
    {
        return IndexFromTailOffset( 0, nullptr );
    }

private:
    int IndexFromHeadOffset( int offset, int* countOut )
    {
        i64 curData = indexData.LOAD_ACQUIRE();
        i32 onePastHeadIndex = curData & 0xFFFFFFFF;
        *countOut = curData >> 32;

        int result = onePastHeadIndex + offset - 1;
        result &= (buffer.capacity - 1);

        return result;
    }

    int IndexFromTailOffset( int offset, int* countOut )
    {
        i64 curData = indexData.LOAD_ACQUIRE();
        i32 onePastHeadIndex = curData & 0xFFFFFFFF;
        int count = curData >> 32;

        int result = onePastHeadIndex - count + offset;
        result &= (buffer.capacity - 1);

        *countOut = count;
        return result;
    }
#if 0
public:
    struct Iterator
    {
        protected:
            SyncRingBuffer<T>& b;
            T* current;
            i32 currentIndex;
            bool forward;

        public:
            Iterator( SyncRingBuffer& buffer_, bool forward_ = true )
                : b( buffer_ )
                  , forward( forward_ )
        {
            currentIndex = forward ? b.TailIndex() : b.IndexFromHeadOffset( 0 );
            current = b.Count() ? &b.buffer.data[currentIndex] : nullptr;
        }

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
                        current = (currentIndex == b._onePastHeadIndex) ? nullptr : &b.buffer.data[currentIndex];
                    }
                    else
                    {
                        currentIndex = (currentIndex - 1) & (b.buffer.capacity - 1);
                        current = (currentIndex < b.TailIndex()) ? nullptr : &b.buffer.data[currentIndex];
                    }
                }

                return *this;
            }
    };
    Iterator MakeIterator( bool forward = true )
    {
        return Iterator( *this, forward );
    }
#endif
};
