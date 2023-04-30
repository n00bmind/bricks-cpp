#pragma once

struct BinaryField
{
    u32 size;
    u8 id;
    u8 _padding[3];
};
static_assert( sizeof(BinaryField) == 8, "Wrong field size" );

inline constexpr sz BinaryFieldSize = offsetof(BinaryField, _padding);
static_assert( BinaryFieldSize == 5, "Wrong field size" );

// TIL about template template parameters ..
// https://stackoverflow.com/questions/38200959/template-template-parameters-without-specifying-inner-type
template <bool RW, template <typename...> typename BufferType = BucketArray>
struct BinaryReflector : public Reflector<RW>
{
    BufferType<u8>* buffer;
    sz bufferHead;

    BinaryReflector( BufferType<u8>* b, Allocator* allocator = CTX_TMPALLOC )
        : Reflector<RW>( allocator )
        , buffer( b )
        , bufferHead( 0 )
    {}

    INLINE void ReadAndAdvance( u8* out, sz size )
    {
        sz copied = buffer->CopyTo( out, size, bufferHead );
        ASSERT( copied == size );
        bufferHead += size;
    }

    INLINE void ReadField( sz offset, BinaryField* fieldOut )
    {
        sz copied = buffer->CopyTo( (u8*)fieldOut, BinaryFieldSize, offset );
        ASSERT( copied == BinaryFieldSize );
    }

    INLINE void WriteField( sz offset, BinaryField const& field )
    {
        buffer->CopyFrom( (u8*)&field, BinaryFieldSize, offset );
    }
};

// TODO Should the reader default to Buffer?
using BinaryReader = BinaryReflector<true>;
using BinaryWriter = BinaryReflector<false>;

template<template <typename...> typename BufferType>
using CustomBinaryReader = BinaryReflector<true, BufferType>;


template <bool RW, template <typename...> typename BufferType>
struct ReflectedTypeInfo< BinaryReflector<RW, BufferType> >
{
    BinaryReflector<RW, BufferType>*   reflector;

    struct Header
    {
        // Total size in bytes of this type, including all metadata
        // move to this point at the end to correct the buffer
        u32 totalSize;
        u8  fieldCount;
        u8  _padding[3];
    } header;
    static_assert( sizeof(Header) == 8, "Wrong header size" );

    static constexpr sz HeaderSize = offsetof(Header, _padding);
    static_assert( HeaderSize == 5, "Wrong header size" );

    u32 startOffset;
    u32 currentFieldSize;


    ReflectedTypeInfo( BinaryReflector<RW, BufferType>* r )
        : reflector( r )
        , header{}
    {
        IF( r->IsWriting )
        {
            startOffset = U32(reflector->buffer->Size());
            // make space to write it back later
            reflector->buffer->PushEmpty( HeaderSize, false );
        }
        else
        {
            startOffset = U32(reflector->bufferHead);
            // decode header
            reflector->ReadAndAdvance( (u8*)&header, HeaderSize );

            sz endOffset = startOffset + header.totalSize;
            if( endOffset > r->buffer->Size() || header.totalSize < HeaderSize )
            {
                LogE( "Core", "Serialised type at %u has an invalid size %u (buffer is %I64d bytes)",
                      startOffset, header.totalSize, r->buffer->Size() );
                reflector->SetError( ReflectResult::BadData );
            }
        }
    }

    ~ReflectedTypeInfo()
    {
        IF( reflector->IsWriting )
        {
            // Finish header
            header.totalSize = U32(reflector->buffer->Size() - startOffset);

            reflector->buffer->CopyFrom( (u8*)&header, HeaderSize, startOffset );
        }
        else
        {
            // can fix ourselves
            if( header.totalSize != 0 )
                reflector->bufferHead = startOffset + header.totalSize;
        }
    }
};

template <bool RW, template <typename...> typename BufferType = BucketArray>
INLINE sz ReflectFieldOffset( BinaryReflector<RW, BufferType>& r )
{
    IF( r.IsWriting )
        return r.buffer->Size();
    else
        return r.bufferHead;
}

// write an initial entry with an empty size
template< typename R > bool ReflectFieldStartWrite( R& r, ReflectedTypeInfo<R>* info, u32 fieldId )
{
    ASSERT( fieldId <= U8MAX, "Id cannot exceed 255 per element" );

    info->header.fieldCount++;

    // Actual size will be computed at the end
    BinaryField newField = { 0, (u8)fieldId };
    r.buffer->Push( (u8*)&newField, BinaryFieldSize );

    return true;
}

// Read the next field, or if the id doesn't match, skip around until we find it
template< typename R > bool ReflectFieldStartRead( R& r, ReflectedTypeInfo<R>* info, u32 fieldId )
{
    BinaryField decodedField;

    // If we're past the current bounds for the type, it's missing (not an error)
    const u32 endOffset = info->startOffset + info->header.totalSize;
    if( r.bufferHead >= endOffset )
        return false;

    r.ReadField( r.bufferHead, &decodedField );
    // Mark the current bounds before potentially skipping around
    info->currentFieldSize = decodedField.size;

    if( decodedField.id == fieldId )
    {
        // We're good to go
    }
    else
    {
        // Next field in the stream is not the one we want
        // Now, the only sensible use case is to use up field ids as needed in an increasing consecutive order, so:
        // · if the decoded field's id is smaller, assume it's a field that was removed, so keep skipping _forward_ until we find the one we want
        // · if the decoded id is bigger, we've probably been reordered, so use the info to return to the start, and loop through
        //   all available fields to find the correct one

        // TODO We could additionally cache all fields we already read so we don't have to read them again
        sz prevFieldOffset = r.bufferHead;

        sz curFieldOffset = (decodedField.id < fieldId)
            ? r.bufferHead + decodedField.size
            : info->startOffset + ReflectedTypeInfo<R>::HeaderSize;

        bool found = false;
        for( int i=0; i < info->header.fieldCount; ++i )
        {
            // If we're right at the end of the type, then we've validly read the last field, so start over from the first one
            if( curFieldOffset == endOffset )
                curFieldOffset = info->startOffset + ReflectedTypeInfo<R>::HeaderSize;

            // Ensure we don't read past the end
            if( curFieldOffset >= endOffset || decodedField.size < BinaryFieldSize )
            {
                // This should only happen when reading the first field from the start (which should be covered by the early out above)
                ASSERT( prevFieldOffset, "We should have read at least one field by now" );

                LogE( "Core", "Serialised field at %u has an invalid size %u (type ends at offset %u)",
                      prevFieldOffset, decodedField.size, endOffset );
                r.SetError( ReflectResult::BadData );
                break;
            }

            r.ReadField( curFieldOffset, &decodedField );

            if( decodedField.id == fieldId )
            {
                r.bufferHead = curFieldOffset;
                found = true;
                break;
            }

            // move to next field
            prevFieldOffset = curFieldOffset;
            curFieldOffset += decodedField.size;
        }
        if( !found )
        {
            // this element is missing, so skip past it
            return false;
        }
    }

    // Finally check the field we're about to read has valid bounds
    if( r.bufferHead + decodedField.size > endOffset || decodedField.size < BinaryFieldSize )
    {
        LogE( "Core", "Serialised field at %u has an invalid size %u (type ends at offset %u)",
              r.bufferHead, decodedField.size, endOffset );
        r.SetError( ReflectResult::BadData );
        return false;
    }

    r.bufferHead += BinaryFieldSize;
    return true;
}

template <bool RW, template <typename...> typename BufferType = BucketArray>
INLINE bool ReflectFieldStart( u32 fieldId, StaticString const& name, ReflectedTypeInfo<BinaryReflector<RW, BufferType>>* info,
                               BinaryReflector<RW, BufferType>& r )
{
    IF( r.IsWriting )
        return ReflectFieldStartWrite( r, info, fieldId );
    else
        return ReflectFieldStartRead( r, info, fieldId );
}

template <bool RW, template <typename...> typename BufferType = BucketArray>
INLINE void ReflectFieldEnd( u32 fieldId, sz fieldStartOffset, ReflectedTypeInfo<BinaryReflector<RW, BufferType>>* info,
                             BinaryReflector<RW, BufferType>& r )
{
    IF( r.IsWriting )
    {
        // Fix up field size
        // final size includes the field metadata
        u32 finalSize = U32(r.buffer->Size() - fieldStartOffset);
        r.WriteField( fieldStartOffset, { finalSize, (u8)fieldId } );
    }
    else 
    {
        // set the read head to ensure it's correct
        r.bufferHead = fieldStartOffset + info->currentFieldSize;
    }
}


template <typename R, typename T>
INLINE ReflectResult ReflectTypeRaw( R& r, T& d )
{
    IF( r.IsWriting )
    {
        r.buffer->Push( (u8*)&d, sizeof(T) );
    }
    else
    {
        if( r.bufferHead + SIZEOF(T) > r.buffer->Size() )
            return { ReflectResult::BufferOverflow };

        r.ReadAndAdvance( (u8*)&d, sizeof(T) );
    }
    return ReflectOk;
}

template <typename R> INLINE ReflectResult Reflect( R& r, bool& d )         { return ReflectTypeRaw( r, d ); }
template <typename R> INLINE ReflectResult Reflect( R& r, char& d )         { return ReflectTypeRaw( r, d ); }
template <typename R> INLINE ReflectResult Reflect( R& r, i8& d )           { return ReflectTypeRaw( r, d ); }
template <typename R> INLINE ReflectResult Reflect( R& r, i16& d )          { return ReflectTypeRaw( r, d ); }
template <typename R> INLINE ReflectResult Reflect( R& r, i32& d )          { return ReflectTypeRaw( r, d ); }
template <typename R> INLINE ReflectResult Reflect( R& r, i64& d )          { return ReflectTypeRaw( r, d ); }
template <typename R> INLINE ReflectResult Reflect( R& r, u8& d )           { return ReflectTypeRaw( r, d ); }
template <typename R> INLINE ReflectResult Reflect( R& r, u16& d )          { return ReflectTypeRaw( r, d ); }
template <typename R> INLINE ReflectResult Reflect( R& r, u32& d )          { return ReflectTypeRaw( r, d ); }
template <typename R> INLINE ReflectResult Reflect( R& r, u64& d )          { return ReflectTypeRaw( r, d ); }
template <typename R> INLINE ReflectResult Reflect( R& r, f32& d )          { return ReflectTypeRaw( r, d ); }
template <typename R> INLINE ReflectResult Reflect( R& r, f64& d )          { return ReflectTypeRaw( r, d ); }

template <typename R>
INLINE ReflectResult ReflectBytesRaw( R& r, void* buffer, sz sizeBytes )
{
    IF( r.IsWriting )
    {
        r.buffer->Push( (u8*)buffer, sizeBytes );
    }
    else
    {
        if( r.bufferHead + sizeBytes > r.buffer->Size() )
            return { ReflectResult::BufferOverflow };

        r.ReadAndAdvance( (u8*)buffer, sizeBytes );
    }
    return ReflectOk;
}

template <typename R, typename T>
INLINE ReflectResult ReflectBytes( R& r, T* buffer, sz bufferLen )
{
    return ReflectBytesRaw( r, (void*)buffer, bufferLen * SIZEOF(T) );
}

template <typename R, typename T, sz N>
ReflectResult Reflect( R& r, T (&d)[N] )
{
    i32 count = N;
    Reflect( r, count );

    // If we're loading an array of a different size, stop / truncate as needed
    IF( r.IsReading )
        count = Min( count, (i32)N );

    ReflectResult result = ReflectOk;
    for( int i = 0; i < count; ++i )
    {
        result = Reflect( r, d[i] );
        if( !result )
            break;
    }
    return result;
}

template <typename R, typename T, sz N>
ReflectResult ReflectArrayPOD( R& r, T (&d)[N] )
{
    i32 count = N;
    Reflect( r, count );

    // If we're loading an array of a different size, stop / truncate as needed
    IF( r.IsReading )
        count = Min( count, (i32)N );

    return ReflectBytes( r, d, count );
}

#define REFLECT_ARRAY( T ) \
    template <typename R, size_t N> \
    ReflectResult Reflect( R& r, T (&d)[N] )

// Faster path for arrays of simple types
// TODO Keep adding suitable types here
REFLECT_ARRAY( bool )       { return ReflectArrayPOD( r, d ); }
REFLECT_ARRAY( char )       { return ReflectArrayPOD( r, d ); }
REFLECT_ARRAY( u8 )         { return ReflectArrayPOD( r, d ); }
REFLECT_ARRAY( u16 )        { return ReflectArrayPOD( r, d ); }
REFLECT_ARRAY( u32 )        { return ReflectArrayPOD( r, d ); }
REFLECT_ARRAY( u64 )        { return ReflectArrayPOD( r, d ); }
//REFLECT_ARRAY( sz )         { return ReflectArrayPOD( r, d ); }       // Already defined
REFLECT_ARRAY( i8 )         { return ReflectArrayPOD( r, d ); }
REFLECT_ARRAY( i16 )        { return ReflectArrayPOD( r, d ); }
REFLECT_ARRAY( i32 )        { return ReflectArrayPOD( r, d ); }
REFLECT_ARRAY( i64 )        { return ReflectArrayPOD( r, d ); }
//REFLECT_ARRAY( int )        { return ReflectArrayPOD( r, d ); }       // Already defined
REFLECT_ARRAY( f32 )        { return ReflectArrayPOD( r, d ); }
REFLECT_ARRAY( f64 )        { return ReflectArrayPOD( r, d ); }

#undef REFLECT_ARRAY

REFLECT_T( Array<T> )
{
    i32 count = d.count;
    Reflect( r, count );

    IF( r.IsReading )
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

    IF( r.IsReading )
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
//REFLECT( Array<sz> )        { return ReflectArrayPOD( r, d ); }
REFLECT( Array<i8> )        { return ReflectArrayPOD( r, d ); }
REFLECT( Array<i16> )       { return ReflectArrayPOD( r, d ); }
REFLECT( Array<i32> )       { return ReflectArrayPOD( r, d ); }
REFLECT( Array<i64> )       { return ReflectArrayPOD( r, d ); }
//REFLECT( Array<int> )       { return ReflectArrayPOD( r, d ); }
REFLECT( Array<f32> )       { return ReflectArrayPOD( r, d ); }
REFLECT( Array<f64> )       { return ReflectArrayPOD( r, d ); }

REFLECT( String )
{
    Reflect( r, d.length );
    Reflect( r, d.flags );

    IF( r.IsReading )
        d.Reset( d.length, d.flags );

    return ReflectBytes( r, d.data, d.length );
}

