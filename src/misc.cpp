
struct CSVCell
{
    // Just a ref to the range inside the source document
    String text;

#if 0
    union
    {
        String strValue = {};
        f32 floatValue;
        i32 intValue;
    };

    enum Type
    {
        Str,
        Float,
        Int,
    };
    Type type;

    ~CSVCell()
    {
        if( type == Str )
            strValue.~String();
    }
#endif
};

struct CSVRow
{
    Array<CSVCell*> cells;
    String text;
    int rowIndex;

    CSVRow( int cellCount ) : cells( cellCount )
    {}
};

struct CSV
{
    BucketArray<String> headers;
    BucketArray<CSVRow> rows;

    BucketArray<CSVCell> cells;
};


// NOTE Uses tmp memory by default!
// TODO Pass some flags for stuff like no header row, etc..
CSV LoadCSV( String contents, char separator = ',', Buffer<char const* const> headerNames = {}, Allocator* allocator = CTX_TMPALLOC )
{
    CSV result = {};

    result.headers.Reset( 32, allocator );

    // Parse headers first
    String headerLine = contents.ConsumeLine();
    while( headerLine )
    {
        String h = headerLine.ConsumeUntil( separator );
        if( h )
            result.headers.Push( h );
        // Skip separator
        headerLine.Consume( 1 );
    }

    if( headerNames )
    {
        // Check against all given names
        for( int i = 0; i < result.headers.count; ++i )
        {
            // TODO log
            ASSERT( result.headers[i] == headerNames[i] );
        }
    }

    result.rows.Reset( 128, allocator );
    result.cells.Reset( (int)result.headers.count * 128, allocator );

    int r = 0;
    while( contents )
    {
        String line = contents.ConsumeLine();

        CSVRow* curRow = nullptr;
        if( line )
        {
            curRow = result.rows.PushInit( (int)result.headers.count );
            curRow->text = line;
            curRow->rowIndex = r++;
        }

        while( line )
        {
            String e = line.ConsumeUntil( separator );

            CSVCell* c = result.cells.PushEmpty();
            c->text = e;
            curRow->cells.Push( c );

#if 0
            c->type = curHeader.Value();
            if( e )
            {
                // Convert to type based on field enum
                switch( curHeader.Value() )
                {
                    case CSVCell::Int:
                    {
                        bool ok = e.ToI32( &c->intValue );
                        // FIXME Log error instead
                        ASSERT( ok );
                    } break;
                    case CSVCell::Float:
                    {
                        bool ok = e.ToF32( &c->floatValue );
                        // FIXME Log error instead
                        ASSERT( ok );
                    } break;
                    case CSVCell::Str:
                    {
                        // NOTE String values will be just refs
                        // unless user code decides to do something about that!
                        c->strValue = e;
                    }
                    break;
                }
            }
#endif
            if( line && line[0] == separator )
                // Skip separator
                line.Consume( 1 );
        }
    }

    return result;
}
