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
    struct ThreadInfo
    {
        Context context;

        char const* name;
        wchar_t* nameOwned;
        Platform::ThreadHandle handle;
        Platform::ThreadFunc* func;
        void* userData;
        u32 id;
    };

    struct State
    {
        ARRAY_DECL(ThreadInfo, 16, liveThreads );
        f64 appStartTimeMillis;
    };
    internal State platformState = {};

    static void InitState( State* state )
    {
        state->appStartTimeMillis = globalPlatform.ElapsedTimeMillis();
    }



    PLATFORM_ALLOC(Alloc)
    {
        return VirtualAlloc( 0, (size_t)sizeBytes, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE );
    }

    PLATFORM_FREE(Free)
    {
        VirtualFree( memoryBlock, 0, MEM_RELEASE );
    }


    PLATFORM_READ_ENTIRE_FILE(ReadEntireFile)
    {
        void* resultData = nullptr;
        sz resultLength = 0;

        HANDLE fileHandle = CreateFile( filename, GENERIC_READ, FILE_SHARE_READ, 0, OPEN_EXISTING, 0, 0 );
        if( fileHandle != INVALID_HANDLE_VALUE )
        {
            sz fileSize;
            if( GetFileSizeEx( fileHandle, (PLARGE_INTEGER)&fileSize ) )
            {
                resultData = ALLOC( allocator, fileSize + 1 );

                if( resultData )
                {
                    DWORD bytesRead;
                    if( ReadFile( fileHandle, resultData, U32(fileSize), &bytesRead, 0 )
                        && (fileSize == bytesRead) )
                    {
                        // Null-terminate to help when handling text files
                        *((u8 *)resultData + fileSize) = '\0';
                        resultLength = fileSize + 1;
                    }
                    else
                    {
                        LogE( "Platform", "ReadFile failed for '%s'", filename );
                        resultData = 0;
                    }
                }
                else
                {
                    LogE( "Platform", "Couldn't allocate buffer for file contents" );
                }
            }
            else
            {
                LogE( "Platform", "Failed querying file size for '%s'", filename );
            }

            CloseHandle( fileHandle );
        }
        else
        {
            LogE( "Platform", "Failed opening file '%s' for reading", filename );
        }

        return { resultData, resultLength };
    }

    PLATFORM_WRITE_ENTIRE_FILE(WriteEntireFile)
    {
        DWORD creationMode = CREATE_NEW;
        if( overwrite ) 
            creationMode = CREATE_ALWAYS;

        HANDLE outFile = CreateFile( filename, GENERIC_WRITE, 0, NULL,
                                    creationMode, FILE_ATTRIBUTE_NORMAL, NULL ); 
        if( outFile == INVALID_HANDLE_VALUE )
        {
            LogE( "Platform", "Could not open '%s' for writing", filename );
            return false;
        }

        bool error = false;
        for( int i = 0; i < chunks.count; ++i )
        {
            Buffer<> const& chunk = chunks[i];
            SetFilePointer( outFile, 0, NULL, FILE_END );

            DWORD bytesWritten;
            if( !WriteFile( outFile, chunk.data, U32( chunk.length ), &bytesWritten, NULL ) )
            {
                LogE( "Platform", "Failed writing %d bytes to '%s'", chunk.length, filename );
                error = true;
                break;
            }
        }

        CloseHandle( outFile );

        return !error;
    }


    internal DWORD WINAPI WorkerThreadProc( LPVOID lpParam )
    {
        ThreadInfo* info = (ThreadInfo*)lpParam;
        // Set up base Context
        // TODO We're gonna need to do this again upon hot reloading for any long-running threads
        Platform::InitContextStack( info->context );

        return (DWORD)info->func( info->userData );
    }

    internal void SetThreadName( ThreadInfo* info )
    {
        // Jump through hoops just to set the fucking name of the fucking thread
        bool didWeActuallySetTheFuckingName = false;

        HMODULE hKernelBase = GetModuleHandleA("KernelBase.dll");
        if( hKernelBase )
        {
            typedef HRESULT (WINAPI *TSetThreadDescription)( HANDLE, PCWSTR );
            auto setThreadDescription = (TSetThreadDescription)(void*)GetProcAddress( hKernelBase, "SetThreadDescription" );
            if( setThreadDescription )
            {
                // Can't be bothered to find out the actual length first so yeah
                constexpr int maxLenName = 64;
                info->nameOwned = ALLOC_ARRAY( CTX_ALLOC, wchar_t, maxLenName );
                int ret = MultiByteToWideChar( CP_UTF8, 0, info->name, -1, info->nameOwned, maxLenName );

                if( ret > 0 )
                {
                    HRESULT result = setThreadDescription( info->handle, info->nameOwned );
                    didWeActuallySetTheFuckingName = !FAILED( result );
                }
                else
                    FREE( CTX_ALLOC, info->nameOwned );
            }
        }

        if( !didWeActuallySetTheFuckingName )
        {
#pragma pack(push, 8)
            struct ThreadName
            {
                DWORD  type;
                LPCSTR name;
                DWORD  id;
                DWORD  flags;
            };
#pragma pack(pop)
            ThreadName tn;
            tn.type  = 0x1000;
            tn.name  = info->name;
            tn.id    = info->id;
            tn.flags = 0;

            __try
            {
                RaiseException( 0x406d1388, 0, sizeof(tn) / sizeof(ULONG_PTR), (ULONG_PTR*)&tn );
            }
            __except(EXCEPTION_EXECUTE_HANDLER)
            { }
        }
    }

    PLATFORM_CREATE_THREAD(CreateThread)
    {
        ThreadInfo* info = platformState.liveThreads.PushEmpty();
        info->name = name;
        info->func = threadFunc;
        info->userData = userdata;
        info->context = threadContext;

        info->handle = ::CreateThread( 0, MEGABYTES(1),
                                       WorkerThreadProc, (LPVOID)info,
                                       0, (LPDWORD)&info->id );

        SetThreadName( info );
        return (Platform::ThreadHandle)info->handle;
    }

    PLATFORM_JOIN_THREAD(JoinThread)
    {
        ThreadInfo* info = platformState.liveThreads.Find( [handle]( ThreadInfo const& i ) { return i.handle == handle; } );
        if( info )
            platformState.liveThreads.Remove( info );
        else
            LogE( "Platform", "Thread with handle 0x%x not found!", handle );

        WaitForSingleObject( handle, INFINITE );

        DWORD exitCode;
        GetExitCodeThread( handle, &exitCode );
        CloseHandle( handle );

        return (int)exitCode;
    }
    
    PLATFORM_GET_THREAD_ID(GetThreadId)
    {
        return GetCurrentThreadId();
    }

    // NOTE This file should be compiled only in the platform layer,
    // so this id will by definition be stable across hot reloads
    static u32 globalMainThreadId = GetThreadId();
    // TODO Test what happens if we decide to restart any threads during hot reloading
    // (I assume the newly started threads will call this all over again and we should be fine?)
    static thread_local u32 globalThreadId = GetThreadId();
    PLATFORM_IS_MAIN_THREAD(IsMainThread)
    {
        return globalThreadId == globalMainThreadId;
    }

    PLATFORM_CREATE_SEMAPHORE(CreateSemaphore)
    {
        HANDLE h = ::CreateSemaphore( NULL, initialCount, MAXLONG, NULL );
        return h;
    }

    PLATFORM_DESTROY_SEMAPHORE(DestroySemaphore)
    {
        CloseHandle( (HANDLE)handle );
    }

    PLATFORM_WAIT_SEMAPHORE(WaitSemaphore)
    {
        WaitForSingleObject( (HANDLE)handle, INFINITE );
    }

    PLATFORM_SIGNAL_SEMAPHORE(SignalSemaphore)
    {
        ReleaseSemaphore( (HANDLE)handle, count, NULL );
    }

    PLATFORM_CREATE_MUTEX(CreateMutex)
    {
        // FIXME Maintain a list of these on the platform state
        static CRITICAL_SECTION mutex;
        InitializeCriticalSectionEx( &mutex, 4000, 0 );   // docs recommend 4000 spincount as sane default
        return &mutex;
    }

    PLATFORM_DESTROY_MUTEX(DestroyMutex)
    {
        DeleteCriticalSection( (CRITICAL_SECTION*)handle );
    }

    PLATFORM_LOCK_MUTEX(LockMutex)
    {
        EnterCriticalSection( (CRITICAL_SECTION*)handle );
    }

    PLATFORM_UNLOCK_MUTEX(UnlockMutex)
    {
        LeaveCriticalSection( (CRITICAL_SECTION*)handle );
    }


    PLATFORM_ELAPSED_TIME_MILLIS(ElapsedTimeMillis)
    {
        persistent f64 perfCounterFrequency = 0;
        if( !perfCounterFrequency )
        {
            LARGE_INTEGER perfCounterFreqMeasure;
            QueryPerformanceFrequency( &perfCounterFreqMeasure );
            perfCounterFrequency = (f64)perfCounterFreqMeasure.QuadPart;
        }

        LARGE_INTEGER counter;
        QueryPerformanceCounter( &counter );
        f64 result = (f64)counter.QuadPart / perfCounterFrequency * 1000
            - platformState.appStartTimeMillis;
        
        return result;
    }

    PLATFORM_SHELL_EXECUTE(ShellExecute)
    {
        int exitCode = -1;
        char outBuffer[2048] = {};

        FILE* pipe = _popen( cmdLine, "rt" );
        if( pipe == NULL )
        {
            _strerror_s( outBuffer, Size( ARRAYCOUNT(outBuffer) ), "Error executing compiler command" );
            LogE( "Platform", outBuffer );
            LogE( "Platform", "\n" );
        }
        else
        {
            while( fgets( outBuffer, I32( ARRAYCOUNT(outBuffer) ), pipe ) )
                globalPlatform.Print( outBuffer );

            if( feof( pipe ) )
                exitCode = _pclose( pipe );
            else
            {
                LogE( "Platform", "Failed reading compiler pipe to the end\n" );
            }
        }

        return exitCode;
    }


    PLATFORM_PRINT(Print)
    {
        va_list args;
        va_start( args, fmt );
        vfprintf( stdout, fmt, args );
        va_end( args );
    }

    PLATFORM_PRINT(Error)
    {
        va_list args;
        va_start( args, fmt );
        vfprintf( stderr, fmt, args );
        va_end( args );
    }

    PLATFORM_PRINT_VA(PrintVA)
    {
        vfprintf( stdout, fmt, args );
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
                LogE( "Platform", "ERROR [%08x] : %s\n", result, out );
        }

        return result;
    }

    // Leave at least one byte for the terminator
    // TODO This is buggy! Check latest version in KoM
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

        LogE( "Platform", "### UNHANDLED EXCEPTION ###\n" );
        LogE( "Platform", "%s", callstack );

        // TODO Write minidump

        return EXCEPTION_EXECUTE_HANDLER;
    }

    ASSERT_HANDLER(DefaultAssertHandler)
    {
        // TODO Logging
        char buffer[256] = {};

        va_list args;
        va_start( args, msg );
        vsnprintf( buffer, ASSERT_SIZE( ARRAYCOUNT(buffer) ), msg, args );
        va_end( args );

        LogE( "Platform", "ASSERTION FAILED! :: \"%s\" (%s@%d)\n", buffer, file, line );

        char callstack[16384];
        DumpCallstackToBuffer( callstack, ARRAYCOUNT(callstack), 2 );

        LogE( "Platform", "%s", callstack );
    }



    void InitGlobalPlatform( Buffer<Logging::ChannelDecl> const& logChannels )
    {
        PlatformAPI win32API = {};
        win32API.Alloc                = Alloc;
        win32API.Free                 = Free;
        win32API.GetContext           = Platform::GetContext;
        win32API.PushContext          = Platform::PushContext;
        win32API.PopContext           = Platform::PopContext;
        win32API.ReadEntireFile       = ReadEntireFile;
        win32API.WriteEntireFile      = WriteEntireFile;
        win32API.CreateThread         = CreateThread;
        win32API.JoinThread           = JoinThread;
        win32API.GetThreadId          = GetThreadId;
        win32API.IsMainThread         = IsMainThread;
        win32API.CreateSemaphore      = CreateSemaphore;
        win32API.DestroySemaphore     = DestroySemaphore;
        win32API.WaitSemaphore        = WaitSemaphore;
        win32API.SignalSemaphore      = SignalSemaphore;
        win32API.CreateMutex          = CreateMutex;
        win32API.DestroyMutex         = DestroyMutex;
        win32API.LockMutex            = LockMutex;
        win32API.UnlockMutex          = UnlockMutex;
        win32API.ElapsedTimeMillis    = ElapsedTimeMillis;
        win32API.ShellExecute         = ShellExecute;
        win32API.Print                = Print;
        win32API.Error                = Error;
        win32API.PrintVA              = PrintVA;
        win32API.ErrorVA              = ErrorVA;
        win32API.DefaultAssertHandler = DefaultAssertHandler;

        Platform::InitGlobalPlatform( win32API, logChannels );

        // TODO At the moment, Platform::InitGlobalPlatform above needs the win32 platform ready to be able to create a thread,
        // while InitState here relies on the globalPlatform being already set up. Clarify this!!
        InitState( &platformState );
    }
}
