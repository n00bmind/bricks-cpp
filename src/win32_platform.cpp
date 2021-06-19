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

#if NON_UNITY_BUILD
#include "win32.h"
#include <DbgHelp.h>
#endif


#define ASSERT_SIZE( value ) \
    (ASSERT( (value) >= 0 ), (size_t)(value))

#define ASSERT_U32( value ) \
    (ASSERT( 0 <= (value) && (value) <= U32MAX ), (u32)(value))


namespace Win32
{
    PLATFORM_ALLOC(Alloc)
    {
        return VirtualAlloc( 0, (size_t)sizeBytes, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE );
    }

    PLATFORM_FREE(Free)
    {
        VirtualFree( memoryBlock, 0, MEM_RELEASE );
    }

    // TODO 
#define PLATFORM_PRINT(name) void name( const char *fmt, ... )
#define PLATFORM_PRINT_VA(name) void name( const char *fmt, va_list args )

    PLATFORM_PRINT(Print)
    {
        va_list args;
        va_start( args, fmt );
        vfprintf( stdout, fmt, args );
        va_end( args );
        fprintf( stdout, "\n" );
    }

    PLATFORM_PRINT(Error)
    {
        va_list args;
        va_start( args, fmt );
        vfprintf( stderr, fmt, args );
        va_end( args );
        fprintf( stderr, "\n" );
    }

    PLATFORM_PRINT_VA(ErrorVA)
    {
        vfprintf( stderr, fmt, args );
    }

    DWORD GetLastError( char* result_string = nullptr, sz result_string_len = 0 )
    {
        DWORD result = ::GetLastError();

        if( result == 0 )
        {
            if( result_string && result_string_len )
                *result_string = 0;
            return result;
        }

        CHAR msg_buffer[2048] = {};
        CHAR* out = result_string ? result_string : msg_buffer;
        DWORD max_out_len = ASSERT_U32( result_string ? result_string_len : ARRAYCOUNT(msg_buffer) );

        DWORD msg_size = FormatMessageA( FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
                                         NULL,
                                         result,
                                         MAKELANGID( LANG_NEUTRAL, SUBLANG_DEFAULT ),
                                         out,
                                         max_out_len,
                                         NULL );
        if( msg_size )
        {
            // Remove trailing newline
            for( int i = 0; i < 2; ++i )
                if( out[msg_size - 1] == '\n' || out[msg_size - 1] == '\r' )
                {
                    msg_size--;
                    out[msg_size] = 0;
                }

            if( !result_string )
                Error( "ERROR [%08x] : %s\n", result, out );
        }

        return result;
    }

    // Leave at least one byte for the terminator
#define APPEND_TO_BUFFER( fmt, ... ) { int msg_len = snprintf( buffer, ASSERT_SIZE( buffer_end - buffer - 1 ), fmt, ##__VA_ARGS__ ); buffer += msg_len; }

    void DumpCallstackToBuffer( char* buffer, sz buffer_length, int ignore_number_of_frames = 0 )
    {
        char* buffer_end = buffer + buffer_length;

        HANDLE hProcess = GetCurrentProcess();

        char exe_path[MAX_PATH];
        {
            if( GetModuleFileName(NULL, exe_path, MAX_PATH) )
            {
                char* p_scan = exe_path;
                char* last_sep = nullptr;
                while( *p_scan != 0 )
                {
                    if( (*p_scan == '\\' || *p_scan == '/') && last_sep != p_scan - 1 )
                        last_sep = p_scan;
                    ++p_scan;
                }
                if( last_sep )
                    *last_sep = 0;

#if 0
                APPEND_TO_BUFFER( "Path for symbols is '%s'\n", exe_path );
#endif
            }
            else
            {
                char error_msg[2048] = {};
                u32 error_code = GetLastError( error_msg, ARRAYCOUNT( error_msg ) );

                APPEND_TO_BUFFER( "<<< GetModuleFileName failed with error 0x%08x ('%s') >>>\n", error_code, error_msg );
            }
        }

        if (!SymInitialize(hProcess, exe_path, TRUE))
        {
            char error_msg[2048] = {};
            u32 error_code = GetLastError( error_msg, ARRAYCOUNT( error_msg ) );

            APPEND_TO_BUFFER( "<<< SymInitialize failed with error 0x%08x ('%s') >>>\n\n", error_code, error_msg );
        }

        char pwd[MAX_PATH] = {};
        GetCurrentDirectory( ASSERT_U32( ARRAYCOUNT(pwd) ), pwd );
        int pwd_len = StringLength( pwd );

        //File::UnfixSlashes( pwd );
        StringToLowercase( pwd );

        void* stacktrace_addresses[32]{};
        CaptureStackBackTrace( ASSERT_U32( ignore_number_of_frames ), 32, stacktrace_addresses, NULL);

        int trace_index = 0;
        char frame_desc_buffer[256];
        while (stacktrace_addresses[trace_index] != nullptr)
        {
            frame_desc_buffer[0] = '\0';

            // Resolve source filename & line
            DWORD lineDisplacement = 0;
            IMAGEHLP_LINE64 lineInfo;
            ZeroMemory(&lineInfo, sizeof(lineInfo));
            lineInfo.SizeOfStruct = sizeof(lineInfo);

            if (SymGetLineFromAddr64(hProcess, (DWORD64)stacktrace_addresses[trace_index], &lineDisplacement, &lineInfo))
            {
                char const* file_path = lineInfo.FileName;

                // Try to convert to relative path using current dir
                StringToLowercase( (char*)lineInfo.FileName );

                if( StringStartsWith( file_path, pwd ) )
                    file_path += pwd_len;
                if( file_path[0] == '\\' || file_path[0] == '/' )
                    file_path++;

                snprintf( frame_desc_buffer, ASSERT_SIZE( ARRAYCOUNT(frame_desc_buffer) ), "%s (%u)", file_path, (u32)lineInfo.LineNumber );
            }
            else
            {
                char error_msg[2048] = {};
                DWORD error_code = GetLastError( error_msg, ARRAYCOUNT( error_msg ) );

                snprintf(frame_desc_buffer, ASSERT_SIZE( ARRAYCOUNT(frame_desc_buffer) ), "[SymGetLineFromAddr64 failed: %s]", error_msg );
            }

            // Resolve symbol name
            const u32 sym_name_len = 256;
            char sym_name_buffer[sym_name_len];
            {
                DWORD64 dwDisplacement = 0;

                char sym_buffer[sizeof(IMAGEHLP_SYMBOL64) + sym_name_len];
                ZeroMemory( &sym_buffer, ASSERT_SIZE( ARRAYCOUNT(sym_buffer) ) );

                IMAGEHLP_SYMBOL64* symbol = (IMAGEHLP_SYMBOL64*)sym_buffer;
                symbol->SizeOfStruct = sizeof(IMAGEHLP_SYMBOL64);
                symbol->MaxNameLength = sym_name_len - 1;

                if (SymGetSymFromAddr64(hProcess, (DWORD64)stacktrace_addresses[trace_index], &dwDisplacement, symbol))
                {
                    SymUnDName64( symbol, sym_name_buffer, ASSERT_U32( ARRAYCOUNT(sym_name_buffer) ) );
                }
                else
                {
                    char error_msg[2048] = {};
                    DWORD error_code = GetLastError( error_msg, ARRAYCOUNT( error_msg ) );
                    snprintf( sym_name_buffer, ASSERT_SIZE( ARRAYCOUNT(sym_name_buffer) ), "[SymGetSymFromAddr64 failed: %s]", error_msg );
                }
            }

            APPEND_TO_BUFFER( "[%2d] %s\n" "\t0x%016llx %s\n", trace_index, sym_name_buffer,
                              (DWORD64)stacktrace_addresses[trace_index], frame_desc_buffer );

            // bail when we hit the application entry-point, don't care much about beyond this level
            if (strstr(sym_name_buffer, "main") != nullptr)
                break;

            trace_index++;
        }

        *buffer = 0;
    }
#undef APPEND_TO_BUFFER

    internal LONG WINAPI ExceptionHandler( LPEXCEPTION_POINTERS exception_pointers )
    {
        char callstack[16384];
        DumpCallstackToBuffer( callstack, ARRAYCOUNT(callstack), 8 );

        Error( "UNHANDLED EXCEPTION\n" );
        Error( "%s", callstack );

        return EXCEPTION_EXECUTE_HANDLER;
    }

}

ASSERT_HANDLER(DefaultAssertHandler)
{
    // TODO Logging
    char buffer[256] = {};

    va_list args;
    va_start( args, msg );
    vsnprintf( buffer, ASSERT_SIZE( ARRAYCOUNT(buffer) ), msg, args );
    va_end( args );

    Win32::Error( "ASSERTION FAILED! :: \"%s\" (%s@%d)\n", buffer, file, line );

    char callstack[16384];
    Win32::DumpCallstackToBuffer( callstack, ARRAYCOUNT(callstack), 2 );

    Win32::Error( "%s", callstack );
}


