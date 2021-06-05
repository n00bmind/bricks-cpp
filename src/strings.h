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

INLINE bool IsNullOrEmpty( char const* str )
{
    return str == nullptr || *str == 0;
}

INLINE bool IsNewline( char c )
{
    bool result = (c == '\n' || c == '\r' );
    return result;
}

INLINE bool IsSpacing( char c )
{
    bool result = (c == ' ' ||
                c == '\t' ||
                c == '\f' ||
                c == '\v');
    return result;
}

INLINE bool IsWhitespace( char c )
{
    bool result = IsSpacing( c )
        || IsNewline( c );
    return result;
}

INLINE bool IsAlpha( char c )
{
    bool result = (c >= 'a' && c <= 'z')
        || (c >= 'A' && c <= 'Z' );
    return result;
}

INLINE bool IsNumber( char c )
{
    bool result = (c >= '0' && c <= '9');
    return result;
}

// Any character, number or symbol
INLINE bool IsWord( char c )
{
    return c > 32 && c < 127;
}

INLINE bool StringsEqual( char const* a, char const* b, bool caseSensitive = true )
{
    if( caseSensitive )
        return strcmp( a, b ) == 0;
    else
    {
        size_t len = strlen( a );
        if( len != strlen( b ) )
            return false;

        for( size_t i = 0; i < len; ++i )
            if( tolower( a[i] ) != tolower( b[i] ) )
                return false;
        return true;
    }
}

INLINE int StringLength( char const* s )
{
    return I32( strlen( s ) );
}

INLINE void StringCopy( char const* src, char* dst, sz dstSize )
{
    strncpy( dst, src, Size( dstSize ) );
}

INLINE bool StringStartsWith( char const* str, char const* find) { return str && find && strstr(str, find) == str; }

// In-place conversion to lowercase. Use length if provided or just advance until a null terminator is found
inline char* StringToLowercase( char* str, int len = 0 )
{
    ASSERT( str, "No string" );

    char* c = str;
    int remaining = len;
    while( (len && remaining) || (!len && *c) )
    {
        int c2 = tolower( *c );
        *c++ = (char)c2;

        remaining--;
    }

    return str;
}


struct String;

// String builder to help compose Strings piece by piece
struct StringBuilder
{
    BucketArray<char> buckets;
    MemoryParams memoryParams;

    // TODO Raise bucket size when we know it works
    StringBuilder( MemoryArena* arena, MemoryParams params = Temporary() )
        //: buckets( arena, 32, params )
        : buckets( arena, 4, params )
        , memoryParams( params )
    {}

    bool Empty() const { return buckets.count == 0; }

    void Append( char const* str, int length = 0 )
    {
        if( !length )
            length = StringLength( str );

        // Not including null terminator
        buckets.Append( str, length );
    }

    void AppendFmt( char const* fmt, ... )
    {
        va_list args;
        va_start( args, fmt );

        int n = 1 + vsnprintf( nullptr, 0, fmt, args );
        char* buf = PUSH_STRING( buckets.arena, n, memoryParams );

        // TODO Does this work??
        vsnprintf( buf, Size( n ), fmt, args );
        va_end( args );

        // TODO We probably want this struct to be made out of irregular buckets that are allocated exactly of the
        // length needed for each append (n above) so we don't need this extra copy
        buckets.Append( buf, n - 1 );
    }

    String ToString( MemoryArena* arena );
};


// Read only string
// TODO Remove all the lazily placed Valid() checks once we've reviewed and ensured null termination
// TODO Once we start usings a global context, remove MemoryArena from all signatures (and pay attention to places that could use tmp mem)
struct String
{
    char const* data;
    i32 length;             // Not including null terminator

    constexpr String()
        : data( "" )
        , length( 0 )
    {}

    explicit String( char const* cString )
        : data( cString )
        , length( I32( strlen( cString ) ) )
    { ASSERT( Valid() ); }

    explicit String( char const* cString, int len )
        : data( cString )
        , length( len )
    { ASSERT( Valid() ); }

    explicit String( Buffer<char> const& buffer )
        : data( buffer )
        , length( I32( buffer.length ) )
    { ASSERT( Valid() ); }

    explicit String( buffer const& buffer )
        : data( (char*)buffer.data )
        , length( I32( buffer.length ) )
    { ASSERT( Valid() ); }

    String const& operator =( char const* cString )
    {
        data = cString;
        length = I32( strlen( cString ) );
        ASSERT( Valid() );
        return *this;
    }

    bool operator ==( const char* cString ) const { return IsEqual( cString ); }
    explicit operator bool() const { return !Empty(); }

    bool Empty() const { return length == 0 || data == nullptr; }
    bool Valid() const { return data == nullptr || data[length] == 0; }
    inline char const* CStr() const { ASSERT( Valid() ); return data ? data : ""; }

    bool IsEqual( const char* cString, bool caseSensitive = true ) const
    {
        if( Empty() )
            return false;

        bool result = false;

        if( caseSensitive )
            result = strncmp( data, cString, Size( length ) ) == 0 && cString[length] == '\0';
        else
        {
            if( (size_t)length != strlen( cString ) )
                return false;

            for( int i = 0; i < length; ++i )
                if( tolower( data[i] ) != tolower( cString[i] ) )
                    return false;
            return true;
        }

        return result;
    }


    static String Clone( char const* src, MemoryArena* arena )
    {
        String result = {};
        result.length = StringLength( src );
        result.data = PUSH_STRING( arena, result.length );
        StringCopy( src, (char*)result.data, result.length );

        ASSERT( result.Valid() );
        return result;
    }

    static String Clone( BucketArray<char> const& src, MemoryArena* arena )
    {
        String result = {};
        result.length = src.count;
        result.data = PUSH_STRING( arena, result.length );
        src.CopyTo( (char*)result.data, result.length );

        ASSERT( result.Valid() );
        return result;
    }

    // Clone src string, while replacing all instances of match with subst
    static String CloneReplace( char const* src, char const* match, char const* subst, MemoryArena* arena )
    {
        int matchLen = StringLength( match );

        // TODO This should use a temporary allocator
        StringBuilder builder( arena );
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

        String result = builder.ToString( arena );
        ASSERT( result.Valid() );
        return result;
    }

    static String FromFormat( MemoryArena* arena, char const* fmt, va_list args )
    {
        va_list argsCopy;
        va_copy( argsCopy, args );

        String result = {};
        result.length = 1 + vsnprintf( nullptr, 0, fmt, args );
        result.data = PUSH_STRING( arena, result.length );

        vsnprintf( (char*)result.data, Size( result.length ), fmt, argsCopy );
        va_end( argsCopy );

        ASSERT( result.Valid() );
        return result;
    }

    static String FromFormat( MemoryArena* arena, char const* fmt, ... )
    {
        va_list args;
        va_start( args, fmt );
        String result = FromFormat( arena, fmt, args );
        va_end( args );

        ASSERT( result.Valid() );
        return result;
    }


    bool IsNewline() const
    {
        return *data == '\n' || (*data == '\r' && *(data + 1) == '\n');
    }

    bool StartsWith( const char* cString ) const
    {
        ASSERT( *cString );
        if( Empty() )
            return false;

        const char* nextThis = data;
        const char* nextThat = cString;

        while( *nextThis && *nextThis == *nextThat )
        {
            nextThis++;
            nextThat++;

            if( *nextThat == '\0' )
                return true;
        }

        return false;
    }

    const char* FindString( const char* cString ) const
    {
        ASSERT( *cString );
        if( Empty() )
            return nullptr;

        const char* prospect = data;
        int remaining = length;
        while( remaining > 0 && *prospect )
        {
            const char* nextThis = prospect;
            const char* nextThat = cString;
            while( *nextThis && *nextThis == *nextThat )
            {
                nextThis++;
                nextThat++;

                if( *nextThat == '\0' )
                    return prospect;
            }

            prospect++;
            remaining--;
        }

        return nullptr;
    }

    int FindLast( char c ) const
    {
        int result = -1;
        for( int i = length - 1; i >= 0; --i )
        {
            if( data[i] == c )
            {
                result = i;
                break;
            }
        }

        return result;
    }

    void Split( BucketArray<String>* result, char separator = ' ' )
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
                    String str =  String( nextData, nextLength );
                    str.CStr();

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
            String str =  String( nextData, nextLength );
            str.CStr();

            result->Push( str );
        }

        ASSERT( Valid() );
    }

    String ConsumeLine()
    {
        int lineLen = length;

        const char* atNL = data + length;
        const char* onePastNL = FindString( "\n" );

        if( onePastNL )
        {
            atNL = onePastNL;
            onePastNL++;
            // Account for stupid Windows "double" NLs
            if( *(atNL - 1) == '\r' )
                atNL--;

            lineLen = I32( onePastNL - data );
        }

        ASSERT( lineLen <= length );
        *(char*)atNL = '\0';
        String line( data, I32( atNL - data ) );

        data = onePastNL;
        length -= lineLen;

        ASSERT( Valid() );
        return line;
    }

    // Consume next word trimming whatever whitespace is there at the beginning
    String ConsumeWord()
    {
        ConsumeWhitespace();

        int wordLen = 0;
        if( *data && length > 0 )
        {
            const char *end = data;
            while( *end && IsWord( *end ) )
                end++;

            wordLen = I32( end - data );
        }

        ASSERT( wordLen <= length );
        *(char*)(data + wordLen) = '\0';
        String result( data, wordLen );

        // Consume stripped part
        wordLen++;
        data += wordLen;
        length -= wordLen;

        ASSERT( Valid() );
        return result;
    }

    int ConsumeWhitespace()
    {
        const char *start = data;
        int remaining = length;

        while( *start && IsWhitespace( *start ) && remaining > 0 )
        {
            start++;
            remaining--;
        }

        int advanced = length - remaining;

        data = start;
        length = remaining;

        ASSERT( Valid() );
        return advanced;
    }

    String Consume( int charCount )
    {
        const char *next = data;
        int remaining = Min( charCount, length );

        while( *next && remaining > 0 )
        {
            next++;
            remaining--;
        }

        int len = I32( next - data );
        ASSERT( len <= length );
        String result( data, len );

        data = next;
        length -= len;

        ASSERT( Valid() );
        return result;
    }

    // FIXME This should actually be totally unnecessary given that now we guarantee null termination!
    // FIXME Make a copy in the stack/temp memory or something
    int Scan( const char *format, ... )
    {
        va_list args;
        va_start( args, format );

        // HACK Not pretty, but it's what we have until C gets a portable bounded vscanf
        char* end = (char*)data + length;
        char endChar = *end;
        *end = '\0';
        int result = vsscanf( data, format, args );
        *end = endChar;

        va_end( args );
        return result;
    }

    String ScanString() const
    {
        String result;

        const char* start = data;
        if( *start == '\'' || *start == '"' )
        {
            const char* next = start + 1;
            int remaining = length - 1;

            while( *next && remaining > 0 )
            {
                if( *next == *start && *(next - 1) != '\\' )
                    break;

                next++;
                remaining--;
            }

            ++start;
            result.data = start;
            result.length = I32( next - start );
        }

        ASSERT( result.Valid() );
        return result;
    }

    bool ToI32( i32* output ) const
    {
        bool result = false;

        char* end = nullptr;
        *output = strtol( data, &end, 0 );

        if( *output == 0 )
            result = (end != nullptr);
        else if( *output == LONG_MAX || *output == LONG_MIN )
            result = (errno != ERANGE);
        else
            result = true;

        return result;
    }

    bool ToU32( u32* output ) const
    {
        bool result = false;

        char* end = nullptr;
        *output = strtoul( data, &end, 0 );

        if( *output == 0 )
            result = (end != nullptr);
        else if( *output == ULONG_MAX )
            result = (errno != ERANGE);
        else
            result = true;

        return result;
    }

    bool ToBool( bool* output ) const
    {
        bool result = false;
        if( IsEqual( "true", false ) || IsEqual( "y", false ) || IsEqual( "yes", false ) || IsEqual( "1" ) )
        {
            result = true;
            *output = true;
        }
        else if( IsEqual( "false", false ) || IsEqual( "n", false ) || IsEqual( "no", false ) || IsEqual( "0" ) )
        {
            result = true;
            *output = false;
        }

        return result;
    }

    // Deep copy
    void CopyTo( String* target, MemoryArena* arena ) const
    {
        target->length = length;
        int len = length + 1;
        target->data = PUSH_ARRAY( arena, char, len );
        PCOPY( data, (char*)target->data, len * SIZEOF(char) );

        ASSERT( target->Valid() );
    }

    // dst buffer will be null terminated always
    int CopyTo( char* dst, int dstLen ) const
    {
        ASSERT( dstLen > 0 );
        int len = Min( length + 1, dstLen );

        strncpy( dst, data, (size_t)len );
        dst[len] = 0;

        return len;
    }
};

constexpr String EmptyString;


inline String StringBuilder::ToString( MemoryArena* arena )
{
    buckets.Append( "\0", 1 );
    return String::Clone( buckets, arena );
}

INLINE bool StringsEqual( String const& a, String const& b )
{
    return a.length == b.length && strncmp( a.data, b.data, Size( a.length ) ) == 0;
}

INLINE bool StringsEqual( String const& a, char const* b )
{
    return strncmp( a.data, b, Size( a.length ) ) == 0 && b[ a.length ] == 0;
}

INLINE u64 StringHash( String const& str )
{
    return Hash64( str.data, str.length );
}

