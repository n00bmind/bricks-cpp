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
            && nums == rhs.nums
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
            && nums == rhs.nums
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

