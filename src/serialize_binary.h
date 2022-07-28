#pragma once

#pragma pack(push, 1)
struct BinaryField
{
    i32 size;
    u8 id;
};
#pragma pack(pop)

constexpr sz binaryFieldSize = sizeof(BinaryField);
static_assert( binaryFieldSize == 5, "Wrong field size" );

// TIL about template template parameters ..
// https://stackoverflow.com/questions/38200959/template-template-parameters-without-specifying-inner-type
template <bool RW, template <typename...> typename BufferType = BucketArray>
struct BinaryReflector : public Reflector<RW>
{
    BufferType<u8>* buffer;
    i32 bufferHead;

    BinaryReflector( BufferType<u8>* b, Allocator* allocator = CTX_TMPALLOC )
        : Reflector( allocator )
        , buffer( b )
        , bufferHead( 0 )
    {
        // TODO How do we control allocations in the buffer (make them use the same allocator this uses) without being super sneaky!?
        ASSERT( RW || b->allocator == allocator, "Buffer and reflector have different allocators" );
    }

    INLINE void ReadAndAdvance( u8* out, int size )
    {
        buffer->CopyTo( out, size, bufferHead );
        bufferHead += size;
    }

    INLINE void ReadField( int offset, BinaryField* fieldOut )
    {
        buffer->CopyTo( (u8*)fieldOut, binaryFieldSize, offset );
    }

    INLINE void ReadFieldAndAdvance( int offset, BinaryField* fieldOut )
    {
        ReadField( offset, fieldOut );
        bufferHead += binaryFieldSize;
    }

    INLINE void WriteField( int offset, BinaryField const& field )
    {
        ASSERT( field.size >= 0 );
        buffer->CopyFrom( (u8*)&field, binaryFieldSize, offset );
    }
};

using BinaryReader = BinaryReflector<true>;
using BinaryWriter = BinaryReflector<false>;



template <bool RW>
struct ReflectedTypeInfo< BinaryReflector<RW> >
{
    BinaryReflector<RW>*   reflector;
    Allocator* allocator;

    // TODO Make a bench so we can compare approaches!
    //u64* fieldInfo;

    i32  startOffset;

#pragma pack(push, 1)
    struct Header
    {
        // Total size in bytes of this type, including all metadata
        // move to this point at the end to correct the buffer
        i32  totalSize;
        // on save, count the number of fields we're accumulating
        i32  firstFieldOffset;
        u8   fieldCount;
    } header;
#pragma pack(pop)
    static_assert( sizeof(Header) == 9, "Wrong header size" );

    ReflectedTypeInfo( BinaryReflector<RW>* r )
        : reflector( r )
        , startOffset( 0 )
        , header{}
    {
        STATIC_IF( r->IsWriting )
        {
            startOffset = reflector->buffer->count;
            // make space to write it back later
            reflector->buffer->PushEmpty( sizeof(Header), false );
        }
        else
        {
            startOffset = reflector->bufferHead;
            // decode header
            reflector->ReadAndAdvance( (u8*)&header, sizeof(Header) );
        }
    }

    ~ReflectedTypeInfo()
    {
        // save how far to jump to find the first field - if the field follows on directly it'll be 4 bytes
        STATIC_IF( reflector->IsWriting )
        {
            ASSERT( header.fieldCount == 0 || header.firstFieldOffset > startOffset, "Bad field offset" );

            header.firstFieldOffset = header.fieldCount > 0 ? header.firstFieldOffset - startOffset : 0;
            header.totalSize = reflector->buffer->count - startOffset;

            // 4 bytes: end point to jump to
            reflector->buffer->CopyFrom( (u8*)&header, sizeof(Header), startOffset );
        }
        else
        {
            // can fix ourselves
            if( header.totalSize != 0 )
                reflector->bufferHead = startOffset + header.totalSize;
        }
    }
};

template <bool RW>
INLINE int ReflectFieldOffset( BinaryReflector<RW>& r )
{
    STATIC_IF( r.IsWriting )
        return r.buffer->count;
    else
        return r.bufferHead;
}

// write an initial entry with an empty size
template< typename R > bool ReflectFieldStartWrite( R& r, ReflectedTypeInfo<R>* info, int fieldId )
{
    ASSERT( fieldId <= U8MAX, "Id cannot exceed 255 per element" );

    // store info to reconstruct the first field
    if ( info->header.fieldCount++ == 0 )
        info->header.firstFieldOffset = r.buffer->count;

    // Actual size will be computed at the end
    BinaryField newField = { 0, (u8)fieldId };
    r.buffer->Push( (u8*)&newField, binaryFieldSize );

    return true;
}

// Read the next field, or if the id doesn't match, find it in the type's field table
template< typename R > bool ReflectFieldStartRead( R& r, ReflectedTypeInfo<R>* info, int fieldId )
{
    BinaryField decodedField;
    // If we're still inside the current bounds for the type, decode normally. Otherwise, it's missing
    if( r.bufferHead < info->startOffset + info->header.totalSize )
        r.ReadField( r.bufferHead, &decodedField );
    else
    {
        // TODO Signal error in the reflector or something
        printf( "Serialise: buffer head at %d overflowed the end of the current type (%d)\n", r.bufferHead, info->startOffset + info->header.totalSize );
        return false;
    }

    if( decodedField.id == fieldId )
    {
        int new_head = r.bufferHead + binaryFieldSize;
        if( new_head > r.buffer->count )
        {
            // TODO Signal error in the reflector or something
            printf( "Serialise: ID %d requests out-of-bounds offset of %d\n", fieldId, new_head );
            return false;
        }
        r.bufferHead += binaryFieldSize;
        return true;
    }
    else
    {
        // Fields have different order
        // use the info to return to the start, and loop through all available fields to find the correct one
        // FIXME Write all per-field info in a footer at the end, and load it into a 256-entry table at the start of each type so we dont have to chase anything

        int fieldStartOffset = info->startOffset + info->header.firstFieldOffset;

        for( int i=0; i < info->header.fieldCount; ++i )
        {
            r.ReadField( fieldStartOffset, &decodedField );

            if( decodedField.id == fieldId )
            {
                int new_head = fieldStartOffset + binaryFieldSize;
                if( new_head > r.buffer->count )
                {
                    // TODO Signal error
                    printf( "Serialise: Out-of-order ID %d requests out-of-bounds offset of %d\n", fieldId, new_head );
                    return false;
                }

                r.bufferHead = new_head;
                return true;
            }

            // move to next field
            fieldStartOffset += decodedField.size;
        }
    }

    // this element is missing, so skip past it
    return false;
}

template <bool RW>
INLINE bool ReflectFieldStart( int fieldId, StaticString const& name, ReflectedTypeInfo<BinaryReflector<RW>>* info, BinaryReflector<RW>& r )
{
    STATIC_IF( r.IsWriting )
        return ReflectFieldStartWrite( r, info, fieldId );
    else
        return ReflectFieldStartRead( r, info, fieldId );
}

template <bool RW>
INLINE void ReflectFieldEnd( int fieldStartOffset, BinaryReflector<RW>& r )
{
    BinaryField decodedField;

    STATIC_IF( r.IsWriting )
    {
        r.ReadField( fieldStartOffset, &decodedField );

        // existing entry should just have a placeholder size
        ASSERT( decodedField.size == 0, "bad entry" );
        // final size includes the field metadata
        int finalSize = r.buffer->count - fieldStartOffset;
        r.WriteField( fieldStartOffset, { finalSize, decodedField.id } );
    }
    else 
    {
        r.ReadField( fieldStartOffset, &decodedField );
        // set the read head to ensure it's correct
        r.bufferHead = fieldStartOffset + decodedField.size;
    }
}


template <typename R, typename T>
ReflectResult ReflectBytes( R& r, T& d )
{
    STATIC_IF( r.IsWriting )
    {
        r.buffer->Push( (u8*)&d, sizeof(T) );
    }
    else
    {
        if( r.bufferHead + SIZEOF(T) > r.buffer->count )
            return { ReflectResult::BufferOverflow };

        r.ReadAndAdvance( (u8*)&d, sizeof(T) );
    }
    return ReflectOk;
}

REFLECT( bool )         { return ReflectBytes( r, d ); }
REFLECT( char )         { return ReflectBytes( r, d ); }
REFLECT( i8 )           { return ReflectBytes( r, d ); }
REFLECT( i16 )          { return ReflectBytes( r, d ); }
REFLECT( i32 )          { return ReflectBytes( r, d ); }
REFLECT( i64 )          { return ReflectBytes( r, d ); }
REFLECT( u8 )           { return ReflectBytes( r, d ); }
REFLECT( u16 )          { return ReflectBytes( r, d ); }
REFLECT( u32 )          { return ReflectBytes( r, d ); }
REFLECT( u64 )          { return ReflectBytes( r, d ); }
REFLECT( f32 )          { return ReflectBytes( r, d ); }
REFLECT( f64 )          { return ReflectBytes( r, d ); }

template <typename R>
ReflectResult ReflectBytesRaw( R& r, void* buffer, int sizeBytes )
{
    STATIC_IF( r.IsWriting )
    {
        r.buffer->Push( (u8*)buffer, sizeBytes );
    }
    else
    {
        if( r.bufferHead + sizeBytes > r.buffer->count )
            return { ReflectResult::BufferOverflow };

        r.ReadAndAdvance( (u8*)buffer, sizeBytes );
    }
    return ReflectOk;
}

template <typename R, typename T>
ReflectResult ReflectBytes( R& r, T* buffer, int bufferLen )
{
    return ReflectBytesRaw( r, (void*)buffer, bufferLen * SIZEOF(T) );
}

REFLECT( String )
{
    i32 length = d.length;
    Reflect( r, length );

    u32 flags = d.flags;
    Reflect( r, flags );

    STATIC_IF( r.IsReading )
        d.Reset( length, flags );

    return ReflectBytes( r, d.data, length );
}

REFLECT_T( Array<T> )
{
    i32 count = d.count;
    Reflect( r, count );

    STATIC_IF( r.IsReading )
    {
        d.Reset( count );
        d.ResizeToCapacity();
    }

    ReflectResult result = ReflectOk;
    for( int i = 0; i < count; ++i )
    {
        result = Reflect( r, d[i] );
        if( !result )
            break;
    }
    return result;
}

template <typename R, typename T>
ReflectResult ReflectArrayPOD( R& r, T& d )
{
    i32 count = d.count;
    Reflect( r, count );

    STATIC_IF( r.IsReading )
    {
        d.Reset( count );
        d.ResizeToCapacity();
    }

    return ReflectBytes( r, d.data, count );
}

// Faster path for arrays of simple types
// TODO Keep adding suitable types here
REFLECT( Array<bool> )      { return ReflectArrayPOD( r, d ); }
REFLECT( Array<char> )      { return ReflectArrayPOD( r, d ); }
REFLECT( Array<u8> )        { return ReflectArrayPOD( r, d ); }
REFLECT( Array<u16> )       { return ReflectArrayPOD( r, d ); }
REFLECT( Array<u32> )       { return ReflectArrayPOD( r, d ); }
REFLECT( Array<u64> )       { return ReflectArrayPOD( r, d ); }
REFLECT( Array<i8> )        { return ReflectArrayPOD( r, d ); }
REFLECT( Array<i16> )       { return ReflectArrayPOD( r, d ); }
REFLECT( Array<i32> )       { return ReflectArrayPOD( r, d ); }
REFLECT( Array<i64> )       { return ReflectArrayPOD( r, d ); }
REFLECT( Array<f32> )       { return ReflectArrayPOD( r, d ); }
REFLECT( Array<f64> )       { return ReflectArrayPOD( r, d ); }

