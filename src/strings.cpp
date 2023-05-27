
void String::Clear()
{
    if( flags & Owned )
    {
        Allocator* allocator = (flags & Temporary) ? CTX_TMPALLOC : CTX_ALLOC;
        FREE( allocator, data );
    }

    data = nullptr;
    length = 0;
    flags &= ~Owned;
}

void String::Reset( int len, u32 flags_ /*= 0*/ )
{
    Clear();
    flags = flags_ | Owned;
    length = len;

    Allocator* allocator = (flags & Temporary) ? CTX_TMPALLOC : CTX_ALLOC;
    data = ALLOC_ARRAY( allocator, char, len + 1, Memory::NoClear() );
    // Null terminate it even if we expect we'll be writing to data later
    ((char*)data)[len] = 0;
}

void String::InternalClone( char const* src, int len /*= 0*/ )
{
    ASSERT( src );

    i32 new_len = len ? len : StringLength( src );
    if( new_len )
    {
        Allocator* allocator = (flags & Temporary) ? CTX_TMPALLOC : CTX_ALLOC;
        char* dst = ALLOC_ARRAY( allocator, char, new_len + 1, Memory::NoClear() );

        COPYP( src, dst, new_len );
        dst[new_len] = 0;

        data = dst;
        length = new_len;
        flags |= Owned;
    }
    else
    {
        data = "";
        length = 0;
    }

    ASSERT( ValidCString() );
}

String String::Clone( BucketArray<char> const& src, bool temporary /*= false*/ )
{
    bool terminated = src.Last() == 0;

    // Constructor already accounts for the terminator space
    String result( (int)(terminated ? src.count - 1 : src.count) );
    src.CopyTo( (char*)result.data, result.length + 1 );

    result.InPlaceModify()[result.length] = 0;
    return result;
}

void String::Split( BucketArray<String>* result, char separator /*= ' '*/ ) const
{
    i32 nextLength = 0;

    char const* s = data;
    char const* nextData = s;
    while( char c = *s++ )
    {
        if( c == separator )
        {
            if( nextLength )
            {
                String str = String::Ref( nextData, nextLength );
                result->Push( str );

                nextData = s;
                nextLength = 0;
            }
        }
        else
            nextLength++;
    }

    // Last piece
    if( nextLength )
    {
        String str = String::Ref( nextData, nextLength );
        result->Push( str );
    }
}

String String::CloneReplace( char const* src, char const* match, char const* subst )
{
    int matchLen = StringLength( match );

    StringBuilder builder;
    int index = 0;
    while( char const* m = strstr( src + index, match ) )
    {
        int subLen = I32( m - (src + index) );
        builder.Append( src + index, subLen );
        builder.Append( subst );

        index += subLen + matchLen;
    }

    int srcLen = StringLength( src );
    ASSERT( index <= srcLen );

    if( index < srcLen )
        builder.Append( src + index, srcLen - index );

    String result = builder.ToString();
    ASSERT( result.ValidCString() );
    return result;
}

