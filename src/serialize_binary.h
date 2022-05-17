#pragma once

// TODO How do we control allocations in the buffer without being super sneaky!?
// TODO Should we have an alternative version of this that creates the buffer in-line instead of having it passed in?
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
    {}
};

using BinaryReader = BinaryReflector<true>;
using BinaryWriter = BinaryReflector<false>;


constexpr sz binaryFieldSize = 5;

INLINE void BinaryFieldEncodeLarge( BucketArray<u8>* buffer, int offset, i32 fieldId, i32 size )
{
    ASSERT( fieldId < U8MAX, "Id cannot exceed 255 per element" );
    ASSERT( size >= 0 );

    ASSERT( offset < buffer->count - binaryFieldSize );
    // We cannot just memcpy, so for simplicity put the bytes in there one by one
    (*buffer)[ offset++ ] = (u8)fieldId;
    (*buffer)[ offset++ ] = (u8)(((u32)size >>  0) & 0xFF);
    (*buffer)[ offset++ ] = (u8)(((u32)size >>  8) & 0xFF);
    (*buffer)[ offset++ ] = (u8)(((u32)size >> 16) & 0xFF);
    (*buffer)[ offset++ ] = (u8)(((u32)size >> 24) & 0xFF);
}
INLINE void BinaryFieldDecodeLarge( BucketArray<u8> const& buffer, int offset, i32* fieldIdOut, i32* sizeOut )
{
    *fieldIdOut = 0;
    buffer.CopyTo( (u8*)fieldIdOut, 1, offset );
    buffer.CopyTo( (u8*)sizeOut,    4, offset + 1 );
}


template <bool RW>
struct ReflectedTypeInfo< BinaryReflector<RW> >
{
    BinaryReflector<RW>*   reflector;
    i32  startOffset;

    // move to this point at the end to correct the buffer
    i32  size;

    // on save, count the number of fields we're accumulating
    i32  fieldCount;
    i32  firstFieldOffset;

    // check against version of saved file - if it's too old, don't attempt to load fields
    //bool versionCheckOk;

    ReflectedTypeInfo( BinaryReflector<RW>* r )
        : reflector( r )
        , startOffset( 0 )
        , size( 0 )
        , fieldCount( 0 )
        , firstFieldOffset( 0 )
    {
        STATIC_IF( r->IsWriting )
        {
            startOffset = reflector->buffer->count;
            // make space to write it back later
            reflector->buffer->PushEmpty( binaryFieldSize + sizeof( size ), false );
        }
        else
        {
            startOffset = reflector->bufferHead;

            // decode the total field count and the jump we need to get to the first field
            BinaryFieldDecodeLarge( *reflector->buffer, startOffset, &fieldCount, &firstFieldOffset );
            reflector->bufferHead += binaryFieldSize;

            // decode end field too
            reflector->buffer->CopyTo( (u8*)&size, sizeof(size), reflector->bufferHead );
            reflector->bufferHead += sizeof(size);
        }
    }

    ~ReflectedTypeInfo()
    {
        // save how far to jump to find the first field - if the field follows on directly it'll be 4 bytes
        STATIC_IF( reflector->IsWriting )
        {
            ASSERT( fieldCount == 0 || firstFieldOffset > startOffset, "Bad field offset" );
            i32 offset = fieldCount > 0 ? firstFieldOffset - startOffset : 0;

            // X bytes of field information
            int bufferOffset = startOffset;
            BinaryFieldEncodeLarge( reflector->buffer, bufferOffset, fieldCount, offset );
            bufferOffset += binaryFieldSize;

            // 4 bytes: end point to jump to
            size = reflector->buffer->count - startOffset;
            (*reflector->buffer)[ bufferOffset++ ] = (u8)(((u32)size >>  0) & 0xFF);
            (*reflector->buffer)[ bufferOffset++ ] = (u8)(((u32)size >>  8) & 0xFF);
            (*reflector->buffer)[ bufferOffset++ ] = (u8)(((u32)size >> 16) & 0xFF);
            (*reflector->buffer)[ bufferOffset++ ] = (u8)(((u32)size >> 24) & 0xFF);
        }
        else
        {
            // can fix ourselves
            if( size != 0 )
                reflector->bufferHead = startOffset + size;
        }
    }
};

template <bool RW>
INLINE u32 ReflectFieldOffset( BinaryReflector<RW>& r )
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
    if ( info->fieldCount++ == 0 )
        info->firstFieldOffset = r.buffer->count;

    r.buffer->Push( (u8*)&fieldId, sizeof(u8) );
    i32 size = 0;
    r.buffer->Push( (u8*)&size, sizeof(u32) );

    return true;
}

// read the next field, or if that is missing, find the next available one
template< typename R > bool ReflectFieldStartRead( R& r, ReflectedTypeInfo<R>* info, int fieldId )
{
    i32 decodedFieldId = 0, decodedFieldSize = 0;
    // If we're still inside the current bounds for the type, decode normally. Otherwise, it's missing
    if( r.bufferHead < info->startOffset + info->size )
        BinaryFieldDecodeLarge( *r.buffer, r.bufferHead, &decodedFieldId, &decodedFieldSize );

    if( decodedFieldId == fieldId )
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

        int fieldStartOffset = info->startOffset + info->firstFieldOffset;

        for( int i=0; i < info->fieldCount; ++i )
        {
            BinaryFieldDecodeLarge( *r.buffer, fieldStartOffset, &decodedFieldId, &decodedFieldSize );

            if( decodedFieldId == fieldId )
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
            fieldStartOffset += decodedFieldSize;
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
    STATIC_IF( r.IsWriting )
    {
        // get the existing entry
        i32 decodedFieldId = 0, decodedFieldSize = 0;
        BinaryFieldDecodeLarge( *r.buffer, fieldStartOffset, &decodedFieldId, &decodedFieldSize );

        // existing entry should be only an id in the high byte
        ASSERT( decodedFieldSize == 0, "bad entry" );

        // the size includes the 4 byte field entry
        int finalSize = r.buffer->count - fieldStartOffset;
        BinaryFieldEncodeLarge( r.buffer, fieldStartOffset, decodedFieldId, finalSize );
    }
    else 
    {
        // get the existing entry
        i32 decodedFieldId = 0, decodedFieldSize = 0;
        BinaryFieldDecodeLarge( *r.buffer, fieldStartOffset, &decodedFieldId, &decodedFieldSize );

        // set the read head to ensure it's correct
        r.bufferHead = fieldStartOffset + decodedFieldSize;
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

        // TODO Abstract these two into a single op so we never forget
        r.buffer->CopyTo( (u8*)&d, sizeof(T), r.bufferHead );
        r.bufferHead += sizeof(T);
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

        r.buffer->CopyTo( (u8*)buffer, sizeBytes, r.bufferHead );
        r.bufferHead += sizeBytes;
    }
    return ReflectOk;
}

template <typename R, typename T>
ReflectResult ReflectBytes( R& r, T* buffer, int bufferLen )
{
    return ReflectBytesRaw( r, (void*)buffer, bufferLen * sizeof(T) );
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

