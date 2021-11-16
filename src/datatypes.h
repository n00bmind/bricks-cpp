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


    static const Array<T, AllocType> Empty;

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

    // NOTE All arrays initialized from a buffer start with count equal to the full capacity
    Array( T* buffer, i32 bufferCount )
        : data( buffer )
        , count( bufferCount )
        , capacity( bufferCount )
    {
        ASSERT( buffer && bufferCount > 0 );
    }

    ~Array()
    {
        if( allocator )
            FREE( allocator, data, memParams );
    }

    operator Buffer<T>()
    {
        return Buffer<T>( data, count );
    }


    T*       begin()         { return data; }
    const T* begin() const   { return data; }
    T*       end()           { return data + count; }
    const T* end() const     { return data + count; }

    T&       First()         { ASSERT( count > 0 ); return data[0]; }
    T const& First() const   { ASSERT( count > 0 ); return data[0]; }
    T&       Last()          { ASSERT( count > 0 ); return data[count - 1]; }
    T const& Last() const    { ASSERT( count > 0 ); return data[count - 1]; }

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
    T* PushEmpty( bool clear = true )
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

template <typename T, typename AllocType>
const Array<T, AllocType> Array<T, AllocType>::Empty = {};


/////     BUCKET ARRAY     /////

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

        operator V() const { ASSERT( IsValid() ); return base->data[index]; }
        V operator *() const { return (V)*this; }

        bool operator ==( IdxBase<V, B> const& other ) const { return base == other.base && index == other.index; }
        bool operator !=( IdxBase<V, B> const& other ) const { return !(*this == other); }


        void Next()
        {
            if( index < base->count - 1 )
                index++;
            else
            {
                base = base->next;
                index = 0;
            }
        }

        IdxBase& operator ++() { Next(); return *this; }
        IdxBase operator ++( int ) { IdxBase result(*this); Next(); return result; }

        void Prev()
        {
            if( index > 0 )
                index--;
            else
            {
                base = base->prev;
                index = base ? base->count - 1 : 0;
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


    static const BucketArray<T> Empty;

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

    Idx         First()          { return { &first, 0 }; }
    ConstIdx    First() const    { return { &first, 0 }; }
    Idx         Last()           { return { last, last->count - 1 }; }
    ConstIdx    Last()  const    { return { last, last->count - 1 }; }
    Idx         End()            { return { last, last->count }; }
    ConstIdx    End()   const    { return { last, last->count }; }

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

    // TODO RemoveOrdered() moves every item after forward one slot (until end of the bucket)

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


    void Append( T const* buffer, int bufferCount )
    {
        int totalCopied = 0;
        Bucket*& bucket = last;

        while( totalCopied < bufferCount )
        {
            int remaining = bufferCount - totalCopied;
            int available = bucket->capacity - bucket->count;

            int copied = Min( remaining, available );
            PCOPY( buffer + totalCopied, bucket->data + bucket->count, copied * SIZEOF(T) );
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

    void CopyTo( T* buffer, sz bufferCount ) const
    {
        ASSERT( count <= bufferCount );

        const Bucket* bucket = &first;
        while( bucket )
        {
            PCOPY( bucket->data, buffer, bucket->count * SIZEOF(T) );
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
            PCOPY( bucket->data, buffer, bucket->count * SIZEOF(T) );
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

template <typename T, typename AllocType>
const BucketArray<T, AllocType> BucketArray<T, AllocType>::Empty = {};


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


    explicit Hashtable( int expectedSize = 0, u32 flags_ = 0, AllocType* allocator_ = CTX_ALLOC, 
                        HashFunc hashFunc_ = DefaultHashFunc<K>, KeysEqFunc eqFunc_ = DefaultEqFunc<K> )
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

// TODO Make this thread-safe by masking the indices (instead of resetting them to 0), and using atomic increment
// We could then easily use this as a fixed-capacity thread-safe queue

// TODO Look at the code inside #ifdef MEM_REPLACE_PLACEHOLDER in https://github.com/cmuratori/refterm/blob/main/refterm_example_source_buffer.c
// for an example of how to speed up linear reads & writes that go beyond the end of the buffer,
// by just mapping the same block of memory twice back to back!

template <typename T, typename AllocType = Allocator>
struct RingBuffer
{
    Array<T, AllocType> buffer;
    i32 onePastHeadIndex;       // Points to next slot to write
    i32 tailIndex;


    RingBuffer()
    {}

    RingBuffer( i32 capacity, AllocType* allocator = CTX_ALLOC, MemoryParams params = {} )
        : buffer( capacity, allocator, params )
        , onePastHeadIndex( 0 )
        , tailIndex( 0 )
    {
        buffer.ResizeToCapacity();
    }

    template <typename AllocType2 = Allocator>
    static RingBuffer FromArray( Array<T, AllocType2> const& a )
    {
        int itemCount = a.count;

        RingBuffer result = {};
        if( itemCount )
        {
            // Make space for one more at the end
            result.buffer.Resize( itemCount + 1 );
            result.buffer.CopyFrom( a );

            result.onePastHeadIndex = itemCount;
            result.tailIndex = 0;
        }

        return result;
    }

    void Clear()
    {
        onePastHeadIndex = 0;
        tailIndex = 0;
    }

    void ClearToZero()
    {
        Clear();
        PZERO( buffer.data, buffer.count * sizeof(T) );
    }

    T* PushEmpty( bool clear = true )
    {
        T* result = buffer.data + onePastHeadIndex++;

        if( onePastHeadIndex == buffer.capacity )
            onePastHeadIndex = 0;
        if( onePastHeadIndex == tailIndex )
        {
            tailIndex++;
            if( tailIndex == buffer.capacity )
                tailIndex = 0;
        }

        if( clear )
            PZERO( result, sizeof(T) );

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
        ASSERT( Count() > 0 );
        int prevTailIndex = tailIndex;
        if( tailIndex != onePastHeadIndex )
        {
            tailIndex++;
            if( tailIndex == buffer.capacity )
                tailIndex = 0;
        }

        return buffer.data[tailIndex];
    }

    T PopHead()
    {
        ASSERT( Count() > 0 );
        int headIndex = onePastHeadIndex;
        if( tailIndex != onePastHeadIndex )
        {
            onePastHeadIndex--;
            if( onePastHeadIndex < 0 )
                onePastHeadIndex = buffer.capacity - 1;
            headIndex = onePastHeadIndex;
        }

        return buffer.data[headIndex];
    }

    int Count() const
    {
        int count = onePastHeadIndex - tailIndex;
        if( count < 0 )
            count += buffer.capacity;

        return count;
    }

    bool Available() const
    {
        return Count() < buffer.capacity - 1;
    }

    bool Contains( const T& item ) const
    {
        return buffer.Contains( item );
    }

    T& FromHead( int offset = 0 )
    {
        ASSERT( offset >= 0, "Only positive offsets" );
        ASSERT( Count() > offset, "Not enough items in buffer" );
        int index = IndexFromHeadOffset( -offset );
        return buffer.data[index];
    }

    T const& FromHead( int offset = 0 ) const
    {
        T& result = ((RingBuffer*)this)->FromHead( offset );
        return (T const&)result;
    }


    struct IteratorInfo
    {
    protected:
        RingBuffer<T>& b;
        T* current;
        i32 currentIndex;
        bool forward;

    public:
        IteratorInfo( RingBuffer& buffer_, bool forward_ = true )
            : b( buffer_ )
            , forward( forward_ )
        {
            currentIndex = forward ? b.tailIndex : b.IndexFromHeadOffset( 0 );
            current = b.Count() ? &b.buffer.data[currentIndex] : nullptr;
        }

        T* operator * () { return current; }
        T* operator ->() { return current; }

        // Prefix
        inline IteratorInfo& operator ++()
        {
            if( current )
            {
                if( forward )
                {
                    currentIndex++;
                    if( currentIndex >= b.buffer.capacity )
                        currentIndex = 0;

                    current = (currentIndex == b.onePastHeadIndex) ? nullptr : &b.buffer.data[currentIndex];
                }
                else
                {
                    currentIndex--;
                    if( currentIndex < 0 )
                        currentIndex = b.buffer.capacity - 1;

                    current = (currentIndex < b.tailIndex) ? nullptr : &b.buffer.data[currentIndex];
                }
            }

            return *this;
        }
    };
    IteratorInfo Iterator( bool forward = true )
    {
        return IteratorInfo( *this, forward );
    }

private:
    int IndexFromHeadOffset( int offset )
    {
        int result = onePastHeadIndex + offset - 1;
        while( result < 0 )
            result += buffer.capacity;
        while( result >= buffer.capacity )
            result -= buffer.capacity;

        return result;
    }
};

