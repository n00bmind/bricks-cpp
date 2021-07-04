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

// Specialised hash & equality implementations for char* strings
template <>
INLINE u64  DefaultHashFunc< char const* >( char const* const& key )                    { return Hash64( key, StringLength( key ) ); }
template <>
INLINE bool DefaultEqFunc< const char* >( char const* const& a, char const* const& b )  { return StringsEqual( a, b ); }



// Read only string
// By default it only just references character data living somewhere else
// It can optionally own the memory too (for easily moving string data around or building formatted strings etc.)

// TODO Write some tests to make sure the default of not copying anything is not a problem when stored inside containers
// TODO Remove all the lazily placed Valid() checks where it makes sense
struct String
{
    enum Flags : u32
    {
        None        = 0,
        Owned       = 1 << 0,
        Temporary   = 1 << 1,
    };

    char const* data;
    i32 length;             // Not including null terminator
    u32 flags;

    constexpr String()
        : data( "" )
        , length( 0 )
        , flags( 0 )
    {}

    String( char const* cString )
        : data( cString )
        , length( StringLength( cString ) )
        , flags( 0 )
    { ASSERT( Valid() ); }

    explicit String( char const* cString, int len )
        : data( cString )
        , length( len )
        , flags( 0 )
    { ASSERT( Valid() ); }

    explicit String( Buffer<char> const& buffer )
        : data( buffer )
        , length( I32( buffer.length ) )
        , flags( 0 )
    { ASSERT( Valid() ); }

    explicit String( buffer const& buffer )
        : data( (char*)buffer.data )
        , length( I32( buffer.length ) )
        , flags( 0 )
    { ASSERT( Valid() ); }

    // Automatically figure out length for string literals
#define STR(x) \
    ([]{ static_assert( true, x ); }, String( x, sizeof(x) - 1 ))

    String( String const& other )
        : flags( other.flags )
    {
        flags &= ~Owned;
        *this = other;
    }

    String( String&& other )
        : flags( other.flags )
    {
        flags &= ~Owned;
        *this = std::move( other );
    }

    ~String()
    { Clear(); }

    void Clear()
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

    String const& operator =( char const* cString )
    {
        Clear();

        data = cString;
        length = StringLength( cString );
        ASSERT( Valid() );
        return *this;
    }

    String const& operator =( String const& other )
    {
        Clear();

        flags = other.flags;
        if( flags & Owned )
            InternalClone( other.data, other.length );
        else
        {
            data = other.data;
            length = other.length;
        }

        return *this;
    }

    String const& operator =( String&& other )
    {
        Clear();

        // Force a copy if they use different memory pools to avoid surprises
        bool force_copy = (other.flags & Owned) && ((flags & Temporary) != (other.flags & Temporary));
        flags = other.flags;

        // Only ever copy if we're gonna own it!
        if( force_copy )
            InternalClone( other.data, other.length );
        else
        {
            data = other.data;
            length = other.length;
        }

        other.flags &= ~Owned;
        other.Clear();

        return *this;
    }

private:
    explicit String( int len, u32 flags_ = 0 )
    {
        flags = flags_ | Owned;
        length = len;

        Allocator* allocator = (flags & Temporary) ? CTX_TMPALLOC : CTX_ALLOC;
        data = ALLOC_ARRAY( allocator, char, len );
    }

    void InternalClone( char const* src, int len = 0 )
    {
        ASSERT( src );

        i32 new_len = len ? len : StringLength( src );
        if( new_len )
        {
            Allocator* allocator = (flags & Temporary) ? CTX_TMPALLOC : CTX_ALLOC;
            char* dst = ALLOC_ARRAY( allocator, char, new_len + 1 );

            PCOPY( src, dst, new_len );
            dst[new_len] = 0;

            data = dst;
            length = new_len;
            flags |= Owned;
        }
    }

public:

    bool operator ==( String const& other ) const { return IsEqual( other.data, other.length ); }
    bool operator ==( const char* cString ) const { return IsEqual( cString ); }
    explicit operator bool() const { return !Empty(); }
    operator char const*() const { return CStr(); }

    bool Empty() const { return length == 0 || data == nullptr; }
    bool Valid() const { return Empty() || data[length] == 0; }
    char const* CStr() const { ASSERT( Valid() ); return data ? data : ""; }

    bool IsEqual( const char* cString, sz len = 0, bool caseSensitive = true ) const
    {
        if( !len )
            len = cString ? StringLength( cString ) : 0;
        if( length != len )
            return false;

        bool result = false;

        if( caseSensitive )
        {
            result = len == 0 || (strncmp( data, cString, (size_t)length ) == 0 && cString[length] == '\0');
        }
        else
        {
            for( int i = 0; i < length; ++i )
                if( tolower( data[i] ) != tolower( cString[i] ) )
                    return false;
            return true;
        }

        return result;
    }
    bool IsEqualIgnoreCase( const char* cString, sz len = 0 ) const
    {
        return IsEqual( cString, len, false );
    }


    static String Clone( char const* src, int len = 0 )
    {
        String result;
        result.InternalClone( src, len );
        return result;
    }

    static String CloneTmp( char const* src, int len = 0 )
    {
        String result;
        result.flags |= Temporary;
        result.InternalClone( src, len );
        return result;
    }

    static String Clone( BucketArray<char> const& src )
    {
        String result( src.count );
        src.CopyTo( (char*)result.data, result.length );

        // TODO Should we auto null terminate here?
        ASSERT( result.Valid() );
        return result;
    }

    static String CloneTmp( BucketArray<char> const& src )
    {
        String result( src.count, Temporary );
        src.CopyTo( (char*)result.data, result.length );

        // TODO Should we auto null terminate here?
        ASSERT( result.Valid() );
        return result;
    }

    // Clone src string, while replacing all instances of match with subst
    static String CloneReplace( char const* src, char const* match, char const* subst );

private:
    static String FromFormat( char const* fmt, va_list args, bool temporary )
    {
        // TODO Apparently I read somewhere that these need to be copied to correctly use them twice?
#if 0
        va_list argsCopy;
        va_copy( argsCopy, args );
#endif
        int len = 1 + vsnprintf( nullptr, 0, fmt, args );

        String result( len, temporary ? Temporary : None );
        vsnprintf( (char*)result.data, (size_t)result.length, fmt, args/*Copy*/ );
#if 0
        va_end( argsCopy );
#endif

        ASSERT( result.Valid() );
        return result;
    }

public:
    static String FromFormat( char const* fmt, va_list args ) { return FromFormat( fmt, args, false ); }
    static String FromFormatTmp( char const* fmt, va_list args ) { return FromFormat( fmt, args, true ); }

    static String FromFormat( char const* fmt, ... )
    {
        va_list args;
        va_start( args, fmt );
        String result = FromFormat( fmt, args, false );
        va_end( args );

        ASSERT( result.Valid() );
        return result;
    }

    static String FromFormatTmp( char const* fmt, ... )
    {
        va_list args;
        va_start( args, fmt );
        String result = FromFormat( fmt, args, true );
        va_end( args );

        ASSERT( result.Valid() );
        return result;
    }


    INLINE u64 Hash() const
    {
        return Empty() ? 0 : Hash64( data, length );
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
        ASSERT( !(flags & Owned) );

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
        ASSERT( !(flags & Owned) );

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
        ASSERT( !(flags & Owned) );

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
        ASSERT( !(flags & Owned) );

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

    // FIXME Make a copy in the stack/temp memory or something
    // (REVIEW! This should actually be totally unnecessary given that now we guarantee null termination!)
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

#if 0
    // Deep copy
    // dst buffer will be null terminated always
    int CopyTo( char* dst, int dstLen ) const
    {
        ASSERT( dstLen > 0 );
        int len = Min( length + 1, dstLen );

        strncpy( dst, data, (size_t)len );
        dst[len] = 0;

        return len;
    }
#endif
};

// Correctly hash Strings
template <>
INLINE u64  DefaultHashFunc< String >( String const& key )  { return key.Hash(); }


INLINE bool StringsEqual( String const& a, String const& b )
{
    return a.length == b.length && strncmp( a.data, b.data, Size( a.length ) ) == 0;
}

INLINE bool StringsEqual( String const& a, char const* b )
{
    return strncmp( a.data, b, Size( a.length ) ) == 0 && b[ a.length ] == 0;
}



// String builder to help compose Strings piece by piece
// NOTE This uses temporary memory by default!
struct StringBuilder
{
    BucketArray<char> buckets;

    // TODO Raise bucket size when we know it works
    explicit StringBuilder( Allocator* allocator = CTX_TMPALLOC )
        //: buckets( 32, params )
        : buckets( 4, allocator )
    {}

    bool Empty() const { return buckets.count == 0; }

    void Append( char const* str, int length = 0 )
    {
        if( !length )
            length = StringLength( str );

        // Not including null terminator
        buckets.Append( str, length );
    }

    void AppendFormat( char const* fmt, ... )
    {
        va_list args;
        va_start( args, fmt );

        int n = 1 + vsnprintf( nullptr, 0, fmt, args );
        char* buf = ALLOC_ARRAY( CTX_TMPALLOC, char, n );

        // TODO Does this work??
        vsnprintf( buf, Size( n ), fmt, args );
        va_end( args );

        // TODO We probably want this struct to be made out of irregular buckets that are allocated exactly of the
        // length needed for each append (n above) so we don't need this extra copy
        buckets.Append( buf, n - 1 );
    }

    String ToString()
    {
        buckets.Append( "\0", 1 );
        return String::Clone( buckets );
    }
    String ToStringTmp()
    {
        buckets.Append( "\0", 1 );
        return String::CloneTmp( buckets );
    }
};


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
    ASSERT( result.Valid() );
    return result;
}

