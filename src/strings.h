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

INLINE int StringLength( char const* s )
{
    return I32( strlen( s ) );
}

INLINE bool StringsEqual( char const* a, char const* b, sz len = 0, bool caseSensitive = true )
{
    if( !len )
        len = a ? StringLength( a ) : 0;

    if( caseSensitive )
    {
        if( a && b )
            return (strncmp( a, b, (size_t)len ) == 0 && b[len] == '\0');
        return (!a && !b);
    }
    else
    {
        if( len != StringLength( b ) )
            return false;

        // TODO SSE
        for( sz i = 0; i < len; ++i )
            if( tolower( a[i] ) != tolower( b[i] ) )
                return false;
        return true;
    }
}

INLINE void StringCopy( char const* src, char* dst, sz dstSize )
{
    strncpy( dst, src, SizeT( dstSize ) );
}

INLINE char const* StringFind( char const* str, char find ) { return strchr( str, find ); }
INLINE char const* StringFind( char const* str, char const* find ) { return strstr( str, find ); }
INLINE char const* StringFind( char const* str, char const* find, int len )
{
    int findLen = StringLength( find );
    if( findLen == 0 )
        return str;

    if( len < findLen )
        return nullptr;

    const char c = find[0];
    while( char const* p = (char const*)memchr( str, c, (size_t)len ) )
    {
        if( (p - str) > (len - findLen) )
            break;

        if( strncmp( p, find, (size_t)findLen ) == 0 )
            return p;

        len -= (int)(p - str);
        str = p;
    }
    return nullptr;
}

// Adapted from https://www.codeproject.com/Articles/383185/SSE-accelerated-case-insensitive-substring-search
inline char const* StringFindIgnoreCase( char const* str, char const* find )
{
    char const *s1, *s2;
    // TODO Supposedly caching tolower/toupper for all ASCII makes this faster, which I find hard to believe but.. bench it!
    char const l = (char)tolower(*find);
    char const u = (char)toupper(*find);
    
    if (!*find)
        return str; // an empty substring matches everything

    // TODO Optimize with SSE (see article above)
    for (; *str; ++str)
    {
        if (*str == l || *str == u)
        {
            s1 = str + 1;
            s2 = find + 1;
            
            while (*s1 && *s2 && tolower(*s1) == tolower(*s2))
                ++s1, ++s2;
            
            if (!*s2)
                return str;
        }
    }
 
    return nullptr;
}

inline char const* StringFindLast( char const* str, char c )
{
    return strrchr( str, c );
}
inline char const* StringFindLast( char const* str, char c, int len )
{
    for( char const* s = str + len - 1; s >= str; --s )
    {
        if( *s == c )
            return s;
    }
    return nullptr;
}

// Adapted from https://stackoverflow.com/questions/1634359/is-there-a-reverse-function-for-strstr
inline char const* StringFindLast( char const* str, char const* find )
{
    if ( !str || !find )
        return nullptr;

    if( *find == '\0' )
        return str + strlen( str );

    char const* result = nullptr;
    for( ;; )
    {
        char const* p = strstr( str, find );
        if( p == nullptr )
            break;

        result = p;
        str = p + 1;
    }

    return result;
}

// TODO Bench against StringFindLast above
inline char const* StringFindSuffix( const char* str, const char* find, int len = 0 )
{
    if ( !str || !find )
        return nullptr;

    size_t strLen = len ? len : strlen( str );
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

INLINE bool StringStartsWith( char const* str, char const* find )
{
    return str && find && strstr( str, find ) == str;
}
INLINE bool StringStartsWith( char const* str, char const* find, int len )
{
    if ( !str || !find )
        return false;

    size_t findLen = strlen( find );
    if( (size_t)len < findLen )
        return false;

    return strncmp( str, find, findLen ) == 0;
}
INLINE bool StringEndsWith( char const* str, char const* find )
{
    return StringFindSuffix( str, find ) != nullptr;
}
INLINE bool StringEndsWith( char const* str, char const* find, int len )
{
    return StringFindSuffix( str, find, len ) != nullptr;
}

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

inline bool StringToI32( char const* str, i32* output, int base = 0 )
{
    bool result = false;

    char* end = nullptr;
    *output = strtol( str, &end, base );

    if( *output == 0 )
        result = (end != nullptr);
    else if( *output == LONG_MAX || *output == LONG_MIN )
        result = (errno != ERANGE);
    else
        result = true;

    return result;
}

inline bool StringToU32( char const* str, u32* output, int base = 0 )
{
    bool result = false;

    char* end = nullptr;
    *output = strtoul( str, &end, base );

    if( *output == 0 )
        result = (end != nullptr);
    else if( *output == ULONG_MAX )
        result = (errno != ERANGE);
    else
        result = true;

    return result;
}

inline bool StringToI64( char const* str, i64* output, int base = 0 )
{
    bool result = false;

    char* end = nullptr;
    *output = strtoll( str, &end, base );

    if( *output == 0 )
        result = (end != nullptr);
    else if( *output == LLONG_MAX || *output == LLONG_MIN )
        result = (errno != ERANGE);
    else
        result = true;

    return result;
}

inline bool StringToU64( char const* str, u64* output, int base = 0 )
{
    bool result = false;

    char* end = nullptr;
    *output = strtoull( str, &end, base );

    if( *output == 0 )
        result = (end != nullptr);
    else if( *output == ULLONG_MAX )
        result = (errno != ERANGE);
    else
        result = true;

    return result;
}

inline bool StringToF32( char const* str, f32* output )
{
    bool result = false;

    char* end = nullptr;
    *output = strtof( str, &end );

    if( *output == 0.f )
        result = (end != nullptr);
    else if( abs( *output ) == HUGE_VALF )
        result = (errno != ERANGE);
    else
        result = true;

    return result;
}

inline bool StringToF64( char const* str, f64* output )
{
    bool result = false;

    char* end = nullptr;
    *output = strtod( str, &end );

    if( *output == 0.f )
        result = (end != nullptr);
    else if( abs( *output ) == HUGE_VAL )
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

// Return whether the full message was appended or we reached the end of the buffer first
inline bool StringAppendToBuffer( char*& buffer, char const* bufferEnd, char const* msg, ... )
{
    if( buffer >= bufferEnd )
        return false;

    // Leave at least one byte for the terminator
    sz remaining = bufferEnd - buffer - 1;
    if( remaining > 0 )
    {
        va_list args;
        va_start( args, msg );
        int msgLen = vsnprintf( buffer, (size_t)remaining, msg, args );
        va_end( args );

        buffer += msgLen;
        if( buffer < bufferEnd )
            return true;
        else
        {
            buffer = (char*)bufferEnd;
            return false;
        }
    }
    return false;
}


// Specialised hash & equality implementations for char* strings
template <>
INLINE u64  DefaultHashFunc< char const* >( char const* const& key )                    { return Hash64( key, StringLength( key ) ); }
template <>
INLINE bool DefaultEqFunc< const char* >( char const* const& a, char const* const& b )  { return StringsEqual( a, b ); }



// Read-only string
// By default it's constructed by cloning any data passed to it (can optionally use temporary memory for that)
// It can also be just a reference to character data somewhere (see Ref() overloads)
// Newly-created owned strings will be null-terminated.

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
        : data( "" ) , length( 0 ) , flags( 0 )
    {}
    explicit constexpr String( u32 flags_ )
        : data( "" ) , length( 0 ) , flags( flags_ )
    {}

    // Conversion constructor from a C string. The given string MUST be null-terminated
    String( char const* cString, u32 flags_ = 0 )
        : flags( flags_ | Owned )
    { InternalClone( cString ); }

    // If given len is 0, the source string MUST be null-terminated
    explicit String( char const* cString, int len, u32 flags_ = 0 )
        : flags( flags_ | Owned )
    { InternalClone( cString, len ); }

    explicit String( char const* cString, char const* cStringEnd, u32 flags_ = 0 )
        : flags( flags_ | Owned )
    { InternalClone( cString, cStringEnd ? I32( cStringEnd - cString ) : StringLength( cString ) ); }

    explicit String( StringBuffer const& buffer, u32 flags_ = 0 )
        : flags( flags_ | Owned )
    { InternalClone( buffer.data, I32(buffer.length) ); }

    // FIXME enable_if T is same size
    template<typename T>
    explicit String( Buffer<T> const& buffer, u32 flags_ = 0 )
        : flags( flags_ | Owned )
    { InternalClone( (char const*)buffer.data, I32(buffer.length) ); }


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

    // Length not counting the null terminator (which will be added)
    void Reset( int len, u32 flags_ = 0 )
    {
        Clear();
        flags = flags_ | Owned;
        length = len;

        Allocator* allocator = (flags & Temporary) ? CTX_TMPALLOC : CTX_ALLOC;
        data = ALLOC_ARRAY( allocator, char, len + 1, Memory::NoClear() );
        // Null terminate it even if we expect we'll be writing to data later
        ((char*)data)[len] = 0;
    }

    // Turn a Ref string into a owned one by allocating and copying
    void MakeOwned()
    {
        if( flags & Owned )
            return;

        InternalClone( data, length );
    }

private:
    explicit String( int len, u32 flags_ = 0 )
    {
        Reset( len, flags );
    }

    // Will copy len chars and append an extra null-terminator at the end
    // If no len is given, assumes src itself is null-terminated and computes its StringLength
    void InternalClone( char const* src, int len = 0 )
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

public:

    bool operator ==( String const& other ) const { return IsEqual( other.data, other.length ); }
    bool operator ==( const char* cString ) const { return IsEqual( cString ); }
    bool operator !=( String const& other ) const { return !(*this == other); }
    bool operator !=( const char* cString ) const { return !(*this == cString); }

    explicit operator bool() const { return !Empty(); }
    operator char const*() const { return c(); }
    char const& operator []( int index ) const { ASSERT( index >= 0 && index < length ); return data[index]; }

    bool Empty() const { return length == 0 || data == nullptr || (length == 1 && data[0] == 0); }
    bool ValidCString() const { return Empty() || data[length] == 0; }

    // Return a valid C String
    // If not null-terminated, makes a new temporary copy with a terminator appended and returns that
    char const* c() const
    {
        if( ValidCString() )
            return data ? data : "";
        else
        {
            // Will already append terminator
            static thread_local String cString;
            cString = String( data, length, Temporary );
            ASSERT( cString.ValidCString() );
            return cString.data;
        }
    }

    StringBuffer ToBuffer() const { return StringBuffer( data, length ); }
    Buffer<u8> ToBufferU8() const { return Buffer<u8>( (u8*)data, length ); }

    bool IsEqual( const char* str, sz len = 0, bool caseSensitive = true ) const
    {
        if( !len )
            len = str ? StringLength( str ) : 0;
        return length == len && (data == str || StringsEqual( data, str, length, caseSensitive ));
    }
    bool IsEqualIgnoreCase( const char* str, sz len = 0 ) const
    {
        return IsEqual( str, len, false );
    }


    static String Ref( char const* src, int len = 0, u32 flags = 0 )
    {
        String result;
        result.data = src;
        result.length = len;
        result.flags = flags & ~Owned;
        return result;
    }
    static String Ref( char const* src, char const* end )
    {
        return Ref( src, end ? I32(end - src) : 0 );
    }
    static String Ref( String const& other )
    {
        return Ref( other.data, other.length );
    }
    // FIXME enable_if T is same size
    template <typename T>
    static String Ref( Buffer<T> const& buffer )
    {
        return Ref( (char const*)buffer.data, I32(buffer.length) );
    }

    static String Clone( char const* src, int len = 0 )
    {
        return String( src, len );
    }
    static String CloneTmp( char const* src, int len = 0 )
    {
        return String( src, len, Temporary );
    }
    static String Clone( char const* src, char const* end )
    {
        return String( src, end ? I32(end - src) : 0 );
    }
    static String CloneTmp( char const* src, char const* end )
    {
        return String( src, end ? I32(end - src) : 0, Temporary );
    }
    template <typename T>
    static String Clone( Buffer<T> const& buffer, int len = 0 )
    {
        return String( (char const*)buffer.data, len ? len : I32(buffer.length) );
    }
    template <typename T>
    static String CloneTmp( Buffer<T> const& buffer, int len = 0 )
    {
        return String( (char const*)buffer.data, len ? len : I32(buffer.length), Temporary );
    }

    // Automatically appends null-terminator
    static String Clone( BucketArray<char> const& src, bool temporary = false )
    {
        bool terminated = src.Last() == 0;

        // Constructor already accounts for the terminator space
        String result( (int)(terminated ? src.count - 1 : src.count) );
        src.CopyTo( (char*)result.data, result.length + 1 );

        result.InPlaceModify()[result.length] = 0;
        return result;
    }
    static String CloneTmp( BucketArray<char> const& src )
    {
        return Clone( src, true );
    }

    // Clone src string, while replacing all instances of match with subst (uses StringBuilder)
    static String CloneReplace( char const* src, char const* match, char const* subst );

private:
    static String FromFormat( char const* fmt, va_list args, bool temporary )
    {
        // TODO I read somewhere that apparently these need to be copied to correctly use them twice?
#if 0
        va_list argsCopy;
        va_copy( argsCopy, args );
#endif
        // String constructor below already accounts for the null terminator
        int len = vsnprintf( nullptr, 0, fmt, args );

        String result( len, temporary ? Temporary : None );
        // Actual string buffer above has one extra character for the null terminator
        vsnprintf( (char*)result.data, (size_t)result.length + 1, fmt, args/*Copy*/ );
#if 0
        va_end( argsCopy );
#endif

        ASSERT( result.ValidCString() );
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
        if( !cString || Empty() )
            return false;
        return StringStartsWith( data, cString, length );
    }

    bool EndsWith ( char const* cString ) const
    {
        if( !cString || Empty() )
            return false;
        return StringEndsWith( data, cString, length );
    }

    const char* FindString( const char* cString ) const
    {
        if( !cString || Empty() )
            return nullptr;
        return StringFind( data, cString, length );
    }

    char const* FindLast( char c ) const
    {
        if( Empty() )
            return nullptr;
        return StringFindLast( data, c, length );
    }


    // NOTE All 'ConsumeX' methods advance the current data pointer a certain number of characters (and reduce length accordingly)
    // This means they are only allowed on Ref Strings!
    // TODO All these need the simd treatment
    String ConsumeLine()
    {
        ASSERT( !(flags & Owned) );

        int lineLen = length;
        char const* atNL = data + length;
        char const* onePastNL = (char*)FindString( "\n" );

        if( onePastNL )
        {
            atNL = onePastNL;
            onePastNL++;
            // Account for 'double' NLs
            if( *(atNL - 1) == '\r' )
                atNL--;
            else if( *onePastNL == '\r' )
                onePastNL++;

            lineLen = I32(onePastNL - data);
        }
        ASSERT( lineLen <= length );

        String line = String::Ref( data, I32(atNL - data) );

        data = onePastNL ? onePastNL : data + lineLen;
        length -= lineLen;

        return line;
    }

    // Consume next word trimming whatever whitespace is there at the beginning
    // This is a WORD in the Vim sense, i.e. anything that's not whitespace is included
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

        String result = String::Ref( data, wordLen );

        // Consume stripped part
        data += wordLen;
        length -= wordLen;

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

        int advancedCount = length - remaining;

        data = start;
        length = remaining;

        return advancedCount;
    }

    String ConsumeUntil( char token )
    {
        ASSERT( !(flags & Owned) );

        const char *start = data;
        int remaining = length;

        while( *start && *start != token && remaining > 0 )
        {
            start++;
            remaining--;
        }

        String result = String::Ref( data, I32(start - data) );

        data = start;
        length = remaining;

        return result;
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
        int advancedCount = I32( next - data );
        ASSERT( advancedCount <= length );

        String result = String::Ref( data, advancedCount );

        data = next;
        length -= advancedCount;

        return result;
    }


    int Scan( const char *format, ... ) const
    {
        va_list args;
        va_start( args, format );

        int result = vsscanf( data, format, args );

        va_end( args );
        return result;
    }

    // Extracts a 'literal string' enclosed in either single or double quotes, checking for and skipping escaped quotes in the middle
    // The returned string doesn't include the quotes
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

            // Don't include starting quote
            ++start;
            result.data = start;
            result.length = I32(next - start);
        }

        return result;
    }

    bool ToI32( i32* output, int base = 0 ) const
    {
        return StringToI32( data, output, base );
    }

    bool ToU32( u32* output, int base = 0 ) const
    {
        return StringToU32( data, output, base );
    }

    bool ToF32( f32* output ) const
    {
        return StringToF32( data, output );
    }

    bool ToBool( bool* output ) const
    {
        return StringToBool( data, output );
    }

    // Find all locations of the given separator in this String and create a bunch of Ref Strings pointing to the substrings
    void Split( BucketArray<String>* result, char separator = ' ' ) const
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


    char* InPlaceModify() { return (char*)data; }

    // In-place conversion to lowercase. Use length if provided or just advance until a null terminator is found
    char* ToLowercase()
    {
        return StringToLowercase( InPlaceModify(), length );
    }

    // dst buffer will be null terminated always
    int CopyTo( char* dst, int dstLen ) const
    {
        ASSERT( dstLen > 0 );
        int copyLen = Min( length, dstLen - 1 );

        COPYP( data, dst, copyLen );
        dst[copyLen] = 0;

        return copyLen;
    }
};

// Correctly hash Strings
template <>
INLINE u64  DefaultHashFunc< String >( String const& key )  { return key.Hash(); }


// Version accepting string literals only, so we can guarantee a Ref is fine always
struct StaticString : public String
{
    constexpr StaticString()
    {}

    template <size_t N>
    constexpr StaticString( char const (&s)[N], u32 flags_ = 0 )
        : String( flags_ & ~Owned )
    {
        data = s;
        // Size of string literal includes the null terminator, so dont count it
        length = (int)N - 1;
    }
    // TODO Check we can replace (& delete) the above with:
    template <size_t N>
    constexpr StaticString( char (&&s)[N], u32 flags_ = 0 )
        : String( flags_ & ~Owned )
    {
        data = s;
        // Size of string literal includes the null terminator, so dont count it
        length = (int)N - 1;
    }

    // NOTE Explicitly forbid non-const char arrays
    // NOTE No protection against passing a const char array allocated in the stack though, so DON'T!
    template<size_t N> StaticString( char (&)[N] ) = delete;
    //StaticString( char const* ) = delete;

    // Always trivially copy
    StaticString( StaticString const& rhs )
    {
        *this = rhs;
    }

protected:
    constexpr StaticString( char const* s, size_t len, u32 flags_ = 0 )
        : String( flags & ~Owned )
    {
        data = s;
        length = (int)len;
    }

    StaticString const& operator =( StaticString const& rhs )
    {
        data   = rhs.data;
        length = rhs.length;
        return *this;
    }

    friend StaticString operator"" _s( const char* s, size_t len );
};

INLINE StaticString operator"" _s( const char *s, size_t len )
{
    return StaticString( s, len );
}


// Version that gets hashed at compile time
struct StaticStringHash : public StaticString
{
    const u64 hash;

    // TODO Check actually constexpr!
    template <size_t N>
    constexpr StaticStringHash( char const (&s)[N], u32 flags_ = 0 )
        : StaticString( s, flags_ )
        //, hash( CONSTEVAL<CompileTimeHash64( s )> )
        , hash( CompileTimeHash64( s ) )
    {}
    // TODO Check we can replace (& delete) the above with:
    template <size_t N>
    constexpr StaticStringHash( char (&&s)[N], u32 flags_ = 0 )
        : StaticString( s, flags_ )
        , hash( CompileTimeHash64( s ) )
    {}

    // TODO We can also accept an l-value constexpr char* if we do something like https://blog.molecular-matters.com/2011/06/22/subtle-differences-in-c/

    // for non-const char arrays like buffers
    template<size_t N> StaticStringHash( char (&)[N] ) = delete;

    constexpr u32 Hash32() const { return (u32)(hash & U32MAX); }
};


// String builder to help compose Strings piece by piece
// NOTE This uses temporary memory by default!
struct StringBuilder
{
    BucketArray<char> buckets;

    explicit StringBuilder( Allocator* allocator = CTX_TMPALLOC )
        : buckets( 32, allocator )
    {}

    bool Empty() const { return buckets.count == 0; }

    void Clear()
    {
        buckets.Clear();
    }

    void Append( char const* str, int length = 0 )
    {
        if( !length )
            length = StringLength( str );

        // Not including null terminator
        buckets.Push( str, length );
    }

    // TODO No way to use this as it will always prefer the above!
    template <size_t N>
    void Append( char (&&str)[N] )
    {
        // Not including null terminator
        buckets.Push( str, N - 1 );
    }

    void AppendFormat( char const* fmt, ... )
    {
        va_list args;
        va_start( args, fmt );

        int n = 1 + vsnprintf( nullptr, 0, fmt, args );
        char* buf = ALLOC_ARRAY( CTX_TMPALLOC, char, n, Memory::NoClear() );

        // TODO Does this work??
        vsnprintf( buf, SizeT( n ), fmt, args );
        va_end( args );

        // TODO We probably want this struct to be made out of irregular buckets that are allocated exactly of the
        // length needed for each append (n above) so we don't need this extra copy
        buckets.Push( buf, n - 1 );
    }

    String ToString()
    {
        buckets.Push( '\0' );
        return String::Clone( buckets );
    }
    String ToStringTmp()
    {
        buckets.Push( '\0' );
        return String::CloneTmp( buckets );
    }
    char const* ToCStringTmp()
    {
        return ToStringTmp().c();
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
    ASSERT( result.ValidCString() );
    return result;
}

