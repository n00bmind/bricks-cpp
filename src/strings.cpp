
// Adapted from https://web.archive.org/web/20100129012106/http://www.merriampark.com/ldc.htm
int StringDistance( char const* a, char const* b, int aLen /*= 0*/, int bLen /*= 0*/ )
{
    //Step 1
    aLen = aLen ? aLen : StringLength( a );
    bLen = bLen ? bLen : StringLength( b );

    if( aLen != 0 && bLen != 0 )
    {
        aLen++;
        bLen++;
        int* d = ALLOC_ARRAY( CTX_TMPALLOC, int, aLen * bLen );

        //Step 2	
        for( int k = 0; k < aLen; k++ )
            d[k] = k;
        for( int k = 0; k < bLen; k++ )
            d[k*aLen] = k;
        //Step 3 and 4	
        for( int i = 1; i < aLen; i++ )
        {
            for( int j = 1; j < bLen; j++ )
            {
                //Step 5
                int cost = ( a[i-1] == b[j-1] ) ? 0 : 1;
                //Step 6             
                d[ j*aLen + i ] = Min( d[ (j-1)*aLen + i ] + 1,
                                       d[ j*aLen + i - 1] + 1,
                                       d[ (j-1)*aLen + i - 1] + cost );
            }
        }
        return d[ aLen*bLen - 1 ];
    }
    else 
        return -1; //a negative return value means that one or both strings are empty.
}

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

