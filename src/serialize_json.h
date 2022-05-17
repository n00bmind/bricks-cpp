#pragma once

// NOTE Uses temporary memory by default!
template <bool RW>
struct JsonReflector : public Reflector<RW>
{
    static constexpr json_object_s EmptyObj = {};
    static constexpr json_value_s  EmptyVal = { (json_object_s*)&EmptyObj, json_type_object };

    struct StackEntry
    {
        json_value_s* value;
        json_object_element_s *lastChild;
    };

    BucketArray<StackEntry> stack;
    StackEntry* head;

    // TODO How do we conditionally define specific members based on RW (a union is one way ofc)
    // Another is something like: std::conditional_t< RW, ReaderImpl, WriterImpl > impl;
    StringBuilder* builder;
    char const* debugJsonString;

    // Stupid SFINAE crap to allow different constructor signatures specific to readers & writers

    // JsonWriter
    template <bool _RW = RW, std::enable_if_t< _RW == RW && !RW >* = nullptr>
    JsonReflector( StringBuilder* sb, Allocator* allocator = CTX_TMPALLOC )
        : JsonReflector( allocator )
    {
        builder = sb;

        // Create an empty root object and add it to the stack
        json_object_s* obj = ALLOC_STRUCT( allocator, json_object_s );
        *obj = { nullptr, 0 };

        json_value_s* root = ALLOC_STRUCT( allocator, json_value_s );
        *root = { obj, json_type_object };

        Push( { root } );
    }

    // JsonReader
    template <bool _RW = RW, std::enable_if_t< _RW == RW && RW >* = nullptr>
    JsonReflector( json_value_s* root, Allocator* allocator = CTX_TMPALLOC )
        : JsonReflector( allocator )
    {
        ASSERT( root );
        Push( root );
    }

    StackEntry* Push( json_value_s* value )
    {
        // TODO Everytime we push a json object, iterate all its children and add them to a hashtable indexed on their name
        // ?? ^ What was this about ??
        head = stack.Push( { value } );
        return head;
    };

    void Pop()
    {
        ASSERT( !stack.Empty(), "JsonReflector stack underrun" );
        stack.Pop();
        head = &stack.Last();
    }

    // TODO Move to json_utils.h
    static json_value_s* FindChild( StaticString const& name, json_object_s* obj )
    {
        json_object_element_s* child = obj->start;
        while( child )
        {
            if( name == child->name->string )
                return child->value;

            child = child->next;
        }
        return nullptr;
    }

protected:

    JsonReflector( Allocator* allocator = CTX_TMPALLOC )
        : Reflector( allocator )
        , stack( 16, allocator )
        , head( nullptr )
    {
    }
};

using JsonReader = JsonReflector<true>;
using JsonWriter = JsonReflector<false>;



template <bool RW>
INLINE u32 ReflectFieldOffset( JsonReflector<RW>& r )
{
    return 0;
}

template <bool RW>
INLINE bool ReflectFieldStart( u16 id, StaticString const& name, ReflectedTypeInfo<JsonReflector<RW>>* info, JsonReflector<RW>& r )
{
    // NOTE Use if constexpr when available to obliterate the check and not-taken branch even in Debug
    STATIC_IF( r.IsWriting )
    {
        // Add a child to current parent object value
        json_object_s* obj = json_value_as_object( r.head->value );

        // If the current value has the dummy sentinel, add a real one
        if( obj == &r.EmptyObj )
        {
            obj = ALLOC_STRUCT( r.allocator, json_object_s );
            *r.head->value = { obj, json_type_object };
            r.head->lastChild = nullptr;
        }

        // No type or contents yet
        json_value_s* val = ALLOC_STRUCT( r.allocator, json_value_s );
        // Make a COPY of the template empty value, so we can keep modifying it as we go along
        *val = r.EmptyVal;
        json_string_s* str = ALLOC_STRUCT( r.allocator, json_string_s );
        *str = { name.data, (size_t)name.length };

        json_object_element_s* e = ALLOC_STRUCT( r.allocator, json_object_element_s );
        *e = { str, val, nullptr };

        if( r.head->lastChild )
            r.head->lastChild->next = e;
        else
            obj->start = e;
        obj->length++;
        r.head->lastChild = e;

        r.Push( { val } );

        return true;
    }
    else
    {
        // Find child with given name under current root object
        json_object_s* obj = json_value_as_object( r.head->value );
        ASSERT( obj, "Current head is not a Json object" );

        json_value_s* child = r.FindChild( name, obj );
        if( child )
        {
            r.Push( child );
        }

        return child != nullptr;
    }
}

void JsonWriteString( JsonWriter& r, bool pretty = true )
{
    JsonWriter::StackEntry const& head = r.stack.First();

    size_t size = 0;
    char *string = nullptr;

    // TODO Set up a json allocator that directly gets the space from the StringBuilder somehow
    if( pretty )
        string = (char*)json_write_pretty( head.value, 0, 0, &size );
    else
        NOT_IMPLEMENTED;

    // TODO Seems like the builder doesnt need to be _in_ the writer at all?
    // Size seems to include null terminator
    r.builder->Append( string, I32(size - 1) );
}

template <bool RW>
INLINE void ReflectFieldEnd( u32 fieldOffset, JsonReflector<RW>& r )
{
    r.Pop();
    
#define DEBUG_JSONWRITER 0
#if DEBUG_JSONWRITER
    r.builder->Clear();
    JsonWriteString( r );
    r.debugJsonString = r.builder->ToStringTmp().c();
    printf( "%s", r.debugJsonString );
    __debugbreak();
#endif
}


REFLECT_SPECIAL_RW( JsonReflector, int )
{
    STATIC_IF( r.IsWriting )
    {
        // TODO Have a switch in the reflector so we can do non-temp serialisation trees
        String numString = String::FromFormatTmp( "%d", d );
        json_number_s* num = ALLOC_STRUCT( r.allocator, json_number_s );
        *num = { numString.data, (size_t)numString.length };

        *r.head->value = { num, json_type_number };
    }
    else
    {
        json_number_s* num = json_value_as_number( r.head->value );
        if( !num )
            return { ReflectResult::BadData };

        if( !StringToI32( num->number, &d ) )
            return { ReflectResult::BadData };
    }
    return ReflectOk;
}

REFLECT_SPECIAL_RW( JsonReflector, String )
{
    STATIC_IF( r.IsWriting )
    {
        json_string_s* str = ALLOC_STRUCT( r.allocator, json_string_s );
        *str = { d.data, (size_t)d.length };

        *r.head->value = { str, json_type_string };
    }
    else
    {
        json_string_s* str = json_value_as_string( r.head->value );
        d = String( str ? str->string : "" );
    }
    return ReflectOk;
}

REFLECT_SPECIAL_RWT( JsonReflector, Array<T> )
{
    STATIC_IF( r.IsWriting )
    {
        json_array_s* array = ALLOC_STRUCT( r.allocator, json_array_s );
        *array = {};

        *r.head->value = { array, json_type_array };

        json_array_element_s* prev = nullptr;
        for( int i = 0; i < d.count; ++i )
        {
            json_value_s* value = ALLOC_STRUCT( r.allocator, json_value_s );
            *value = r.EmptyVal;

            r.Push( { value } );
            ReflectResult result = Reflect( r, d[i] );
            if( !result )
                return result;

            json_array_element_s* e = ALLOC_STRUCT( r.allocator, json_array_element_s );
            *e = { value, nullptr };

            if( i == 0 )
                array->start = e;
            else
                prev->next = e;

            r.Pop();

            prev = e;
        }

        array->length = (size_t)d.count;
    }
    else
    {
        json_array_s* array = json_value_as_array( r.head->value );
        if( !array )
            return { ReflectResult::BadData };

        int count = (int)array->length;
        d.Reset( count );
        d.ResizeToCapacity();

        json_array_element_s* el = array->start;
        for( int i = 0; i < count; ++i, el = el->next )
        {
            if( !el )
                return { ReflectResult::BadData };

            r.Push( { el->value } );

            ReflectResult result = Reflect( r, d[i] );
            if( !result )
                return result;

            r.Pop();
        }
    }
    return ReflectOk;
}


