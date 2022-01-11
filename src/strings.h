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

INLINE char const* StringFind( char const* str, char find ) { return strchr( str, find ); }
INLINE char const* StringFind( char const* str, char const* find ) { return strstr( str, find ); }
inline char const* StringFindSuffix( const char* str, const char* find )
{
    if ( !str || !find )
        return nullptr;

    size_t strLen = strlen( str );
    size_t sufLen = strlen( find );
    if ( sufLen >  strLen )
        return nullptr;

    if ( strncmp( str + strLen - sufLen, find, sufLen ) == 0 )
	{
		// Found it
		return str + strLen - sufLen;
	}

	// Fail
	return nullptr;
}

INLINE bool StringStartsWith( char const* str, char const* find ) { return str && find && strstr( str, find ) == str; }
INLINE bool StringEndsWith( char const* str, char const* find ) { return StringFindSuffix( str, find ) != nullptr; }

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

inline bool StringToI32( char const* str, i32* output )
{
    bool result = false;

    char* end = nullptr;
    *output = strtol( str, &end, 0 );

    if( *output == 0 )
        result = (end != nullptr);
    else if( *output == LONG_MAX || *output == LONG_MIN )
        result = (errno != ERANGE);
    else
        result = true;

    return result;
}

inline bool StringToU32( char const* str, u32* output )
{
    bool result = false;

    char* end = nullptr;
    *output = strtoul( str, &end, 0 );

    if( *output == 0 )
        result = (end != nullptr);
    else if( *output == ULONG_MAX )
        result = (errno != ERANGE);
    else
        result = true;

    return result;
}

inline bool StringToBool( char const* str, bool* output )
{
    bool result = false;
    if( StringsEqual( str, "true", false ) || StringsEqual( str, "y", false ) || StringsEqual( str, "yes", false ) || StringsEqual( str, "1" ) )
    {
        result = true;
        *output = true;
    }
    else if( StringsEqual( str, "false", false ) || StringsEqual( str, "n", false ) || StringsEqual( str, "no", false ) || StringsEqual( str, "0" ) )
    {
        result = true;
        *output = false;
    }

    return result;
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

    constexpr String( u32 flags_ = 0 )
        : data( "" )
        , length( 0 )
        , flags( flags_ )
    {}

    String( char const* cString, u32 flags_ = 0 )
        : flags( flags_ & ~Owned )
    { InternalClone( cString ); }

    explicit String( char const* cString, int len, u32 flags_ = 0 )
        : flags( flags_ & ~Owned )
    { InternalClone( cString, len ); }

    explicit String( char const* cString, char const* cStringEnd, u32 flags_ = 0 )
        : flags( flags_ & ~Owned )
    { InternalClone( cString, cStringEnd ? I32( cStringEnd - cString ) : StringLength( cString ) ); }

    explicit String( StringBuffer const& buffer, u32 flags_ = 0 )
        : flags( flags_ & ~Owned )
    // Don't count null terminator
    { InternalClone( buffer.data, I32(buffer.length - 1) ); }

    explicit String( Buffer<char> const& buffer, u32 flags_ = 0 )
        : flags( flags_ & ~Owned )
    // Don't count null terminator
    { InternalClone( buffer.data, I32(buffer.length - 1) ); }

    explicit String( Buffer<> const& buffer, u32 flags_ = 0 )
        : flags( flags_ & ~Owned )
    // Don't count null terminator
    { InternalClone( (char const*)buffer.data, I32(buffer.length - 1) ); }


    String( String const& other )
        : flags( 0 )
    {
        *this = other;
    }

    String( String&& other )
        : flags( 0 )
    {
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

        InternalClone( cString );
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
            // If we haven't forced a copy, don't delete any memory owned by the source
            other.flags &= ~Owned;
        }

        other.Clear();

        return *this;
    }

private:
    // Length not counting the null terminator (which will be added)
    explicit String( int len, u32 flags_ = 0 )
    {
        flags = flags_ | Owned;
        length = len;

        Allocator* allocator = (flags & Temporary) ? CTX_TMPALLOC : CTX_ALLOC;
        data = ALLOC_ARRAY( allocator, char, len + 1 );
        // Null terminate it even if we expect this to be overwritten
        ((char*)data)[len] = 0;
    }

    void InternalClone( char const* src, int len = 0 )
    {
        ASSERT( src );

        i32 new_len = len ? len : StringLength( src );
        if( new_len )
        {
            Allocator* allocator = (flags & Temporary) ? CTX_TMPALLOC : CTX_ALLOC;
            char* dst = ALLOC_ARRAY( allocator, char, new_len + 1 );

            COPYP( src, dst, new_len );
            dst[new_len] = 0;

            data = dst;
            length = new_len;
            flags |= Owned;
        }

        ASSERT( Valid() );
    }

public:

    bool operator ==( String const& other ) const { return length == other.length && IsEqual( other.data, other.length ); }
    bool operator ==( const char* cString ) const { return IsEqual( cString ); }
    explicit operator bool() const { return !Empty(); }
    operator char const*() const { return CStr(); }

    bool Empty() const { return length == 0 || data == nullptr; }
    bool Valid() const { return Empty() || data[length] == 0; }
    char const* CStr() const { ASSERT( Valid() ); return data ? data : ""; }

    StringBuffer ToBuffer() const { return StringBuffer( data, length ); }
    Buffer<u8> ToBufferU8() const { return Buffer<u8>( (u8*)data, length ); }

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


    static String Ref( char const* src, int len = 0 )
    {
        String result;
        result.data = src;
        result.length = len;
        return result;
    }
    static String Ref( String const& other )
    {
        return Ref( other.data, other.length );
    }
    static String Ref( Buffer<> const& buffer )
    {
        // Don't count terminator
        return Ref( (char const*)buffer.data, I32(buffer.length - 1) );
    }

    static String Clone( char const* src, int len = 0 )
    {
        String result( src, len );
        return result;
    }
    static String CloneTmp( char const* src, int len = 0 )
    {
        String result( src, len, Temporary );
        return result;
    }

    static String Clone( char const* src, char const* end )
    {
        String result( src, end ? I32( end - src ) : 0 );
        return result;
    }
    static String CloneTmp( char const* src, char const* end )
    {
        String result( src, end ? I32( end - src ) : 0, Temporary );
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
        // String constructor below already accounts for the null terminator
        int len = vsnprintf( nullptr, 0, fmt, args );

        String result( len, temporary ? Temporary : None );
        // Actual string buffer has one extra character for the null terminator
        vsnprintf( (char*)result.data, (size_t)result.length + 1, fmt, args/*Copy*/ );
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

        return result;
    }

    static String FromFormatTmp( char const* fmt, ... )
    {
        va_list args;
        va_start( args, fmt );
        String result = FromFormat( fmt, args, true );
        va_end( args );

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
        if( Empty() )
            return false;

        return StringStartsWith( data, cString );
    }

    bool EndsWith ( char const* cString ) const
    {
        if( Empty() )
            return false;

        // TODO Could be faster
        return StringEndsWith( data, cString );
    }

    // FIXME Use faster string functions
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
                    String str =  String::Ref( nextData, nextLength );
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
            String str =  String::Ref( nextData, nextLength );
            str.CStr();

            result->Push( str );
        }

        ASSERT( Valid() );
    }

    char* InPlaceModify() { return (char*)data; }

    // NOTE All 'Consume' methods mutate the current String by inserting terminators at the consumed boundaries
    // (ideally we'd like to be able to return non-null terminated Strings to avoid modifying the input)
    String ConsumeLine()
    {
        int lineLen = length;
        char* atNL = &InPlaceModify()[length];
        char* onePastNL = (char*)FindString( "\n" );

        if( onePastNL )
        {
            atNL = onePastNL;
            onePastNL++;
            // Account for 'double' NLs
            if( *(atNL - 1) == '\r' )
                atNL--;
            else if( *onePastNL == '\r' )
                onePastNL++;

            lineLen = I32( onePastNL - data );
        }
        ASSERT( lineLen <= length );

        *atNL = '\0';
        String line = String::Ref( data, I32( atNL - data ) );

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
        InPlaceModify()[wordLen] = '\0';
        String result = String::Ref( data, wordLen );

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
        String result = String::Ref( data, len );

        data = next;
        length -= len;

        ASSERT( Valid() );
        return result;
    }

    int Scan( const char *format, ... )
    {
        va_list args;
        va_start( args, format );

        int result = vsscanf( data, format, args );

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
        return StringToI32( data, output );
    }

    bool ToU32( u32* output ) const
    {
        return StringToU32( data, output );
    }

    bool ToBool( bool* output ) const
    {
        return StringToBool( data, output );
    }

    // In-place conversion to lowercase. Use length if provided or just advance until a null terminator is found
    char* ToLowercase()
    {
        return StringToLowercase( (char*)data, length );
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

// Automatically figure out length for string literals
// TODO constexpr
INLINE String operator"" _str( const char *s, size_t len )
{
    return String::Ref( s, I32(len) );
}

// Correctly hash Strings
template <>
INLINE u64  DefaultHashFunc< String >( String const& key )  { return key.Hash(); }



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

