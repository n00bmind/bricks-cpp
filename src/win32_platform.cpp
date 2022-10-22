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
        ThreadInfo liveThreads[16];
        sz threadCount;
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

    internal u64 FiletimeToPOSIX( FILETIME ft )
    {
        LARGE_INTEGER date, adjust;
        date.HighPart = (LONG)ft.dwHighDateTime;
        date.LowPart = ft.dwLowDateTime;

        // 100-nanoseconds = milliseconds * 10000
        adjust.QuadPart = 11644473600000LL * 10000;

        // removes the diff between 1970 and 1601
        date.QuadPart -= adjust.QuadPart;

        // converts back from 100-nanoseconds to seconds
        return (u64)date.QuadPart / 10000000;
    }
    PLATFORM_GET_FILE_ATTRIBUTES(GetFileAttributes)
    {
        *out = {};

        WIN32_FILE_ATTRIBUTE_DATA data = {};
        if( !GetFileAttributesExA( filename, GetFileExInfoStandard, &data ) )
            return false;

        out->sizeBytes = ((size_t)data.nFileSizeHigh << 32) | data.nFileSizeLow;
        out->modifiedTimePosix = FiletimeToPOSIX( data.ftLastWriteTime );

        return true;
    }

    PLATFORM_READ_ENTIRE_FILE(ReadEntireFile)
    {
        u8* resultData = nullptr;
        sz resultLength = 0;

        HANDLE fileHandle = CreateFile( filename, GENERIC_READ, FILE_SHARE_READ, 0, OPEN_EXISTING, 0, 0 );
        if( fileHandle != INVALID_HANDLE_VALUE )
        {
            sz fileSize;
            if( GetFileSizeEx( fileHandle, (PLARGE_INTEGER)&fileSize ) )
            {
                resultLength = nullTerminate ? fileSize + 1 : fileSize;
                resultData = (u8*)ALLOC( allocator, resultLength );

                if( resultData )
                {
                    DWORD bytesRead;
                    if( ReadFile( fileHandle, resultData, U32(fileSize), &bytesRead, 0 )
                        && (fileSize == bytesRead) )
                    {
                        // Null-terminate to help when handling text files
                        if( nullTerminate )
                            *(resultData + fileSize) = '\0';
                    }
                    else
                    {
                        LogE( "Platform", "ReadFile failed for '%s'", filename );
                        resultData = nullptr;
                        resultLength = 0;
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

        return Buffer<u8>( resultData, resultLength );
    }

    PLATFORM_WRITE_FILE_CHUNKS(WriteFileChunks)
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
        ASSERT( platformState.threadCount < ARRAYCOUNT(platformState.liveThreads), "Too many threads" );

        ThreadInfo* info = &platformState.liveThreads[platformState.threadCount++];
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
        ThreadInfo* info = nullptr;
        for( ThreadInfo& ti : platformState.liveThreads )
            if( ti.handle == handle )
            {
                info = &ti;
                break;
            }

        if( info )
            *info = platformState.liveThreads[platformState.threadCount--];
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

    int Utf8ToWideString( const char* in, wchar_t* out, sz outSize )
    {
        return MultiByteToWideChar( CP_UTF8, 0, in, -1, out, (int)(outSize / sizeof(wchar_t)) );
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
            _strerror_s( outBuffer, SizeT( ARRAYCOUNT(outBuffer) ), "Error executing compiler command" );
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

    PLATFORM_TEST_CONNECTIVITY(TestConnectivity)
    {
        // https://docs.microsoft.com/en-us/windows/win32/api/wininet/nf-wininet-internetgetconnectedstateex
        DWORD flags;
        TCHAR name[ 512 ];

        if( InternetGetConnectedStateEx( &flags, name, (DWORD)ARRAYCOUNT( name ), 0 ) )
        {
            if( !(flags & INTERNET_CONNECTION_OFFLINE) )
                return true;
        }

        return false;
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

    DWORD GetLastError( char* resultString = nullptr, sz resultStringLen = 0 )
    {
        ASSERT( !resultString || resultStringLen );

        DWORD result = ::GetLastError();
        if( result == 0 )
        {
            if( resultString && resultStringLen )
                *resultString = 0;
            return result;
        }

        CHAR msgBuffer[2048] = {};
        CHAR* out = resultString ? resultString : msgBuffer;
        DWORD maxOutLen = ASSERT_U32( resultString ? resultStringLen : ARRAYCOUNT(msgBuffer) );

        DWORD msgSize = FormatMessageA( FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
                                        NULL,
                                        result,
                                        MAKELANGID( LANG_NEUTRAL, SUBLANG_DEFAULT ),
                                        out,
                                        maxOutLen,
                                        NULL );
        if( msgSize )
        {
            // Remove trailing newline
            for( int i = 0; i < 2; ++i )
                if( out[msgSize - 1] == '\n' || out[msgSize - 1] == '\r' )
                {
                    msgSize--;
                    out[msgSize] = 0;
                }

            if( !resultString )
                LogE( "Platform", "ERROR [%08x] : %s\n", result, out );
        }

        return result;
    }

    int CaptureCallstack( void* framesOut[], sz framesOutLen, int framesIgnoredCount = 0 )
    {
        int frameCount = CaptureStackBackTrace( (DWORD)framesIgnoredCount, (DWORD)framesOutLen, framesOut, NULL );
        return frameCount;
    }

    sz ResolveCallstack( void* const frames[], int framesLen, char* bufferOut, sz bufferOutLen, char const* lineSeparator = nullptr )
    {
        if( !lineSeparator )
            lineSeparator = "\n";

        HANDLE hProcess = GetCurrentProcess();
        char const* const bufferStart = bufferOut;
        char const* const bufferEnd = bufferOut + bufferOutLen;

        char exePath[MAX_PATH];
        {
            if( GetModuleFileNameA( NULL, exePath, MAX_PATH ) )
            {
                char* pScan = exePath;
                char* lastSep = nullptr;
                while( *pScan != 0 )
                {
                    if( (*pScan == '\\' || *pScan == '/') && lastSep != pScan - 1 )
                        lastSep = pScan;
                    ++pScan;
                }
                if( lastSep )
                    *lastSep = 0;

                StringAppendToBuffer( bufferOut, bufferEnd, "Path for symbols is '%s'%s", exePath, lineSeparator );
            }
            else
            {
                char errorMsg[1024];
                u32 errorCode = GetLastError( errorMsg, ARRAYCOUNT(errorMsg) );
                StringAppendToBuffer( bufferOut, bufferEnd, "<<< GetModuleFileName failed with error 0x%08x ('%s') >>>%s", errorCode, errorMsg, lineSeparator );
            }
        }

        if( !SymInitialize( hProcess, exePath, TRUE ) )
        {
            char errorMsg[1024];
            u32 errorCode = GetLastError( errorMsg, ARRAYCOUNT(errorMsg) );
            StringAppendToBuffer( bufferOut, bufferEnd, "<<< SymInitialize failed with error 0x%08x ('%s') >>>%s%s", errorCode, errorMsg, lineSeparator, lineSeparator);
        }

#if 0
        // TODO Do we need this?
        char cwd[MAX_PATH];
        StringCopy( cwd, sizeof(cwd), File::GetBasePath() );
        File::UnfixSlashes( cwd );
        ToLowercase( cwd );
#endif

        char frameDescBuffer[256];
        for( int frameIdx = 0; frameIdx < framesLen; ++frameIdx )
        {
            if( frames[frameIdx] == nullptr )
                continue;

            frameDescBuffer[0] = '\0';

            // Resolve source filename & line
            DWORD lineDisplacement = 0;
            IMAGEHLP_LINE64 lineInfo;
            ZeroMemory( &lineInfo, sizeof(lineInfo) );
            lineInfo.SizeOfStruct = sizeof(lineInfo);

            if( SymGetLineFromAddr64( hProcess, (DWORD64)frames[frameIdx], &lineDisplacement, &lineInfo ) )
            {
                char const* filePath = lineInfo.FileName;

#if 0
                // Try to convert to relative path using the project's base path
                ToLowercase( (char*)lineInfo.FileName );
                int idx = StringFind( filePath, cwd );

                if( idx != -1 )
                    filePath += idx + strlen( cwd );
                if( filePath[0] == '\\' || filePath[0] == '/' )
                    filePath++;
#endif

                snprintf( frameDescBuffer, sizeof(frameDescBuffer), "%s (%lu)", filePath, lineInfo.LineNumber );
            }
            else
            {
                char errorMsg[1024];
                u32 errorCode = GetLastError( errorMsg, ARRAYCOUNT(errorMsg) );
                snprintf( frameDescBuffer, sizeof(frameDescBuffer), "[SymGetLineFromAddr64 failed: %s]", errorMsg );
            }

            // Resolve symbol name
            char symNameBuffer[256];
            const int symNameLen = sizeof(symNameBuffer);
            {
                DWORD64 dwDisplacement = 0;

                char symBuffer[sizeof(IMAGEHLP_SYMBOL64) + symNameLen];
                ZeroMemory( &symBuffer, sizeof(symBuffer) );

                IMAGEHLP_SYMBOL64* symbol = (IMAGEHLP_SYMBOL64*)symBuffer;
                symbol->SizeOfStruct = sizeof(IMAGEHLP_SYMBOL64);
                symbol->MaxNameLength = symNameLen - 1;

                if( SymGetSymFromAddr64( hProcess, (DWORD64)frames[frameIdx], &dwDisplacement, symbol ) )
                {
                    SymUnDName64( symbol, symNameBuffer, symNameLen );
                }
                else
                {
                    char errorMsg[1024];
                    u32 errorCode = GetLastError( errorMsg, ARRAYCOUNT(errorMsg) );
                    snprintf( symNameBuffer, symNameLen, "[SymGetSymFromAddr64 failed: %s]", errorMsg );
                }
            }

            bool appended = StringAppendToBuffer( bufferOut, bufferEnd, "[%2d] %s%s" "\t0x%016llx %s%s", frameIdx, symNameBuffer, lineSeparator,
                                                  (DWORD64)frames[frameIdx], frameDescBuffer, lineSeparator );

            // bail when we hit the application entry-point, don't care much about beyond this level
            if( !appended
                || strstr(symNameBuffer, "EntryPoint::") != nullptr
                || strstr(symNameBuffer, "main") != nullptr
                || strstr(symNameBuffer, "wWinMain") != nullptr )
                break;
        }

        *bufferOut++ = 0;
        return bufferOut - bufferStart;
    }

    int DumpCallstackToBuffer( char* bufferOut, sz bufferLen, int framesIgnoredCount = 0, char const* lineSeparator = nullptr )
    {
        void* addresses[64]{};
        int frameCount = CaptureCallstack( addresses, ARRAYCOUNT(addresses), framesIgnoredCount );

        ResolveCallstack( addresses, frameCount, bufferOut, bufferLen, lineSeparator );
        return frameCount;
    }


    internal LONG WINAPI ExceptionHandler( LPEXCEPTION_POINTERS exceptionPointers )
    {
        char callstack[16384];
        DumpCallstackToBuffer( callstack, ARRAYCOUNT(callstack), 8 );

        LogE( "Platform", "### UNHANDLED EXCEPTION ###\n%s", callstack );

        // Write minidump
        u32 errorCode = 0;
        char errorMsg[1024] = {};
        bool minidumpWritten = false;
        static constexpr char const* filename = "minidump.dmp";

        HANDLE hFile = CreateFileA( filename, GENERIC_READ | GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL );
        if( hFile != NULL && hFile != INVALID_HANDLE_VALUE )
        {
            MINIDUMP_EXCEPTION_INFORMATION mdei;

            mdei.ThreadId = GetCurrentThreadId();
            mdei.ExceptionPointers = exceptionPointers;
            mdei.ClientPointers = FALSE;

            // NOTE Never needed this
            static constexpr bool full_mem = false;

            MINIDUMP_TYPE mdt = (MINIDUMP_TYPE)(
                                    full_mem
                                    ? (MiniDumpWithFullMemory | MiniDumpIgnoreInaccessibleMemory | MiniDumpWithThreadInfo)
                                    : (MiniDumpWithIndirectlyReferencedMemory | MiniDumpWithDataSegs | MiniDumpWithThreadInfo)
                                    );

            if( MiniDumpWriteDump( GetCurrentProcess(), GetCurrentProcessId(), hFile, mdt, &mdei, 0, 0 ) )
                minidumpWritten = true;
            else
                errorCode = GetLastError( errorMsg, ARRAYCOUNT(errorMsg) );

            CloseHandle(hFile);
        }
        else
            errorCode = GetLastError( errorMsg, ARRAYCOUNT(errorMsg) );

        if( minidumpWritten )
            LogI( "Platform", "Minidump written to %s", filename );
        else
            LogE( "Platform", "Failed to write minidump with error 0x%08x ('%s')", errorCode, errorMsg );

        return EXCEPTION_EXECUTE_HANDLER;
    }

    ASSERT_HANDLER(DefaultAssertHandler)
    {
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


    void InstallDefaultCrashHandler()
    {
        SetUnhandledExceptionFilter( ExceptionHandler );
    }

    void InitGlobalPlatform( Buffer<Logging::ChannelDecl> const& logChannels )
    {
        Platform::API win32API = {};
        win32API.Alloc                = Alloc;
        win32API.Free                 = Free;
        win32API.GetContext           = Platform::GetContext;
        win32API.PushContext          = Platform::PushContext;
        win32API.PopContext           = Platform::PopContext;
        win32API.GetFileAttributes    = GetFileAttributes;
        win32API.ReadEntireFile       = ReadEntireFile;
        win32API.WriteFileChunks      = WriteFileChunks;
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
        win32API.TestConnectivity     = TestConnectivity;
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
