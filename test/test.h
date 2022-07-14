#pragma once

struct SerialTypeSimple
{
    int num;
};

REFLECT( SerialTypeSimple )
{
    BEGIN_FIELDS;
    FIELD( 1, num );
    return ReflectOk;
}


struct SerialTypeComplex
{
    SerialTypeSimple simple;
    Array<int> nums;
    String str;

    bool operator ==( SerialTypeComplex const& rhs )
    {
        return EQUAL( simple, rhs.simple )
            && EQUALP( nums.data, rhs.nums.data, nums.count * SIZEOF(int) )
            && str == rhs.str;
    }
};

REFLECT( SerialTypeComplex )
{
    BEGIN_FIELDS;
    FIELD( 1, simple );
    FIELD( 2, nums );
    FIELD( 3, str );
    return ReflectOk;
}

struct SerialTypeComplex2
{
    SerialTypeSimple simple;
    Array<int> nums;
    String str;

    bool operator ==( SerialTypeComplex const& rhs )
    {
        return EQUAL( simple, rhs.simple )
            && EQUALP( nums.data, rhs.nums.data, nums.count * SIZEOF(int) )
            && str == rhs.str;
    }
};

REFLECT( SerialTypeComplex2 )
{
    BEGIN_FIELDS;
    FIELD( 1, simple );
    FIELD( 3, str );
    FIELD( 2, nums );
    return ReflectOk;
}

struct SerialTypeComplex3
{
    SerialTypeSimple simple;
    String str;

    bool operator ==( SerialTypeComplex const& rhs )
    {
        return EQUAL( simple, rhs.simple )
            && str == rhs.str;
    }
};

REFLECT( SerialTypeComplex3 )
{
    BEGIN_FIELDS;
    FIELD( 1, simple );
    FIELD( 3, str );
    return ReflectOk;
}


struct SerialTypeDeep
{
    SerialTypeComplex complexx;
    int n;

    bool operator ==( SerialTypeDeep const& rhs )
    {
        return n == rhs.n && complexx == rhs.complexx;
    }
};

REFLECT( SerialTypeDeep )
{
    BEGIN_FIELDS;
    FIELD( 1, complexx );
    FIELD( 2, n );
    return ReflectOk;
}

struct SerialTypeDeeper
{
    SerialTypeDeep deep;
    String someText;

    bool operator ==( SerialTypeDeeper const& rhs )
    {
        return someText == rhs.someText && deep == rhs.deep;
    }
};

REFLECT( SerialTypeDeeper )
{
    BEGIN_FIELDS;
    FIELD( 1, deep );
    FIELD( 2, someText );
    return ReflectOk;
}

struct SerialTypeChunky
{
    Array<SerialTypeDeeper> deeper;

    bool operator ==( SerialTypeChunky const& rhs )
    {
        if( deeper.count != rhs.deeper.count )
            return false;
        for( int i = 0; i < deeper.count; ++i )
            if( !(deeper[i] == rhs.deeper[i]) )
                return false;
        return true;
    }
};

REFLECT( SerialTypeChunky )
{
    BEGIN_FIELDS;
    FIELD( 1, deeper );
    return ReflectOk;
}

