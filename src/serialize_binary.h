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
        : Reflector( allocator )
        , buffer( b )
        , bufferHead( 0 )
    {}

    INLINE void ReadAndAdvance( u8* out, sz size )
    {
        buffer->CopyTo( out, size, bufferHead );
        bufferHead += size;
    }

    INLINE void ReadField( sz offset, BinaryField* fieldOut )
    {
        buffer->CopyTo( (u8*)fieldOut, BinaryFieldSize, offset );
    }

    INLINE void ReadFieldAndAdvance( sz offset, BinaryField* fieldOut )
    {
        ReadField( offset, fieldOut );
        bufferHead += BinaryFieldSize;
    }

    INLINE void WriteField( sz offset, BinaryField const& field )
    {
        buffer->CopyFrom( (u8*)&field, BinaryFieldSize, offset );
    }
};

using BinaryReader = BinaryReflector<true>;
using BinaryWriter = BinaryReflector<false>;



template <bool RW>
struct ReflectedTypeInfo< BinaryReflector<RW> >
{
    BinaryReflector<RW>*   reflector;

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

    struct FieldInfo
    {
        u32 fieldStartOffset;
        u8 fieldId;
    };
    //FieldInfo* fieldInfo;

    u32  startOffset;


    ReflectedTypeInfo( BinaryReflector<RW>* r )
        : reflector( r )
        , header{}
    {
        STATIC_IF( r->IsWriting )
        {
            startOffset = U32(reflector->buffer->count);
            // make space to write it back later
            reflector->buffer->PushEmpty( HeaderSize, false );
        }
        else
        {
            startOffset = U32(reflector->bufferHead);
            // decode header
            reflector->ReadAndAdvance( (u8*)&header, HeaderSize );
        }
    }

    ~ReflectedTypeInfo()
    {
        STATIC_IF( reflector->IsWriting )
        {
            // Finish header
            header.totalSize = U32(reflector->buffer->count - startOffset);

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

template <bool RW>
INLINE sz ReflectFieldOffset( BinaryReflector<RW>& r )
{
    STATIC_IF( r.IsWriting )
        return r.buffer->count;
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

// Read the next field, or if the id doesn't match, find it in the type's field table
template< typename R > bool ReflectFieldStartRead( R& r, ReflectedTypeInfo<R>* info, u32 fieldId )
{
    BinaryField decodedField;

    // TODO Review how much sense these bounds checks make

    // If we're still inside the current bounds for the type, decode normally. Otherwise, it's missing
    if( r.bufferHead < info->startOffset + info->header.totalSize )
        r.ReadField( r.bufferHead, &decodedField );
    else
    {
        // TODO Signal error in the reflector or something
        printf( "Serialise: buffer head at %I64d overflowed the end of the current type (%d)\n", r.bufferHead, info->startOffset + info->header.totalSize );
        return false;
    }

    if( decodedField.id == fieldId )
    {
        sz new_head = r.bufferHead + BinaryFieldSize;
        if( new_head > r.buffer->count )
        {
            // TODO Signal error in the reflector or something
            printf( "Serialise: ID %u requests out-of-bounds offset of %I64d\n", fieldId, new_head );
            return false;
        }
        r.bufferHead += BinaryFieldSize;
        return true;
    }
    else
    {
        // Fields have different order
        // use the info to return to the start, and loop through all available fields to find the correct one
        // FIXME Write all per-field info in a footer at the end, and load it into a 256-entry table at the start of each type so we dont have to chase anything

        sz fieldStartOffset = info->startOffset + ReflectedTypeInfo<R>::HeaderSize;

        for( int i=0; i < info->header.fieldCount; ++i )
        {
            r.ReadField( fieldStartOffset, &decodedField );

            if( decodedField.id == fieldId )
            {
                sz new_head = fieldStartOffset + BinaryFieldSize;
                if( new_head > r.buffer->count )
                {
                    // TODO Signal error
                    printf( "Serialise: Out-of-order ID %u requests out-of-bounds offset of %I64d\n", fieldId, new_head );
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
INLINE bool ReflectFieldStart( u32 fieldId, StaticString const& name, ReflectedTypeInfo<BinaryReflector<RW>>* info, BinaryReflector<RW>& r )
{
    STATIC_IF( r.IsWriting )
        return ReflectFieldStartWrite( r, info, fieldId );
    else
        return ReflectFieldStartRead( r, info, fieldId );
}

template <bool RW>
INLINE void ReflectFieldEnd( u32 fieldId, sz fieldStartOffset, BinaryReflector<RW>& r )
{
    STATIC_IF( r.IsWriting )
    {
        // final size includes the field metadata
        u32 finalSize = U32(r.buffer->count - fieldStartOffset);
        r.WriteField( fieldStartOffset, { finalSize, (u8)fieldId } );
    }
    else 
    {
        // TODO Could use the field-table info here instead of reading the field back
        BinaryField decodedField;
        r.ReadField( fieldStartOffset, &decodedField );
        // set the read head to ensure it's correct
        r.bufferHead = fieldStartOffset + decodedField.size;
    }
}


template <typename R, typename T>
INLINE ReflectResult ReflectBytes( R& r, T& d )
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

template <typename R> INLINE ReflectResult Reflect( R& r, bool& d )         { return ReflectBytes( r, d ); }
template <typename R> INLINE ReflectResult Reflect( R& r, char& d )         { return ReflectBytes( r, d ); }
template <typename R> INLINE ReflectResult Reflect( R& r, i8& d )           { return ReflectBytes( r, d ); }
template <typename R> INLINE ReflectResult Reflect( R& r, i16& d )          { return ReflectBytes( r, d ); }
template <typename R> INLINE ReflectResult Reflect( R& r, i32& d )          { return ReflectBytes( r, d ); }
template <typename R> INLINE ReflectResult Reflect( R& r, i64& d )          { return ReflectBytes( r, d ); }
template <typename R> INLINE ReflectResult Reflect( R& r, u8& d )           { return ReflectBytes( r, d ); }
template <typename R> INLINE ReflectResult Reflect( R& r, u16& d )          { return ReflectBytes( r, d ); }
template <typename R> INLINE ReflectResult Reflect( R& r, u32& d )          { return ReflectBytes( r, d ); }
template <typename R> INLINE ReflectResult Reflect( R& r, u64& d )          { return ReflectBytes( r, d ); }
template <typename R> INLINE ReflectResult Reflect( R& r, f32& d )          { return ReflectBytes( r, d ); }
template <typename R> INLINE ReflectResult Reflect( R& r, f64& d )          { return ReflectBytes( r, d ); }

template <typename R>
INLINE ReflectResult ReflectBytesRaw( R& r, void* buffer, sz sizeBytes )
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
INLINE ReflectResult ReflectBytes( R& r, T* buffer, sz bufferLen )
{
    return ReflectBytesRaw( r, (void*)buffer, bufferLen * SIZEOF(T) );
}

REFLECT( String )
{
    Reflect( r, d.length );
    Reflect( r, d.flags );

    STATIC_IF( r.IsReading )
        d.Reset( d.length, d.flags );

    return ReflectBytes( r, d.data, d.length );
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

