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


//
// Compiler stuff
//

#if defined(__clang__) || defined(__GNUC__)   // Put this first so we account for clang-cl too

#define COMPILER_LLVM 1

#if defined(__amd64__) || defined(__x86_64__)
    #define ARCH_X64 1
#elif defined(__arm__)
    #define ARCH_ARM 1
#endif

#elif _MSC_VER

#define COMPILER_MSVC 1

#if defined(_M_X64) || defined(_M_AMD64)
    #define ARCH_X64 1
#elif defined(_M_ARM)
    #define ARCH_ARM 1
#endif

#else

#error Compiler not supported!

#endif //__clang__


#if COMPILER_MSVC
#define LIB_EXPORT extern "C" __declspec(dllexport)
#else
#define LIB_EXPORT extern "C" __attribute__((visibility("default")))
#endif


struct Allocator;
template <typename T, typename AllocType = Allocator> struct Array;

#define PLATFORM_ALLOC(name) void* name( sz sizeBytes, u32 flags )
typedef PLATFORM_ALLOC(PlatformAllocFunc);
#define PLATFORM_FREE(name) void name( void* memoryBlock )
typedef PLATFORM_FREE(PlatformFreeFunc);


// Defined by the application
struct Context;

#define PLATFORM_PUSH_CONTEXT(name) void name( Context const& newContext )
typedef PLATFORM_PUSH_CONTEXT(PlatformPushContextFunc);
#define PLATFORM_POP_CONTEXT(name) void name()
typedef PLATFORM_POP_CONTEXT(PlatformPopContextFunc);


// TODO Do something better for paths
#define PLATFORM_PATH_MAX 1024

#define PLATFORM_GET_ABSOLUTE_PATH(name) bool name( char const* filename, char* outBuffer, sz outBufferLen )
typedef PLATFORM_GET_ABSOLUTE_PATH(PlatformGetAbsolutePathFunc);
// Returned buffer data must be null-terminated
#define PLATFORM_READ_ENTIRE_FILE(name) buffer name( char const* filename, Allocator* allocator )
typedef PLATFORM_READ_ENTIRE_FILE(PlatformReadEntireFileFunc);
#define PLATFORM_WRITE_ENTIRE_FILE(name) bool name( char const* filename, Array<buffer> const& chunks, bool overwrite )
typedef PLATFORM_WRITE_ENTIRE_FILE(PlatformWriteEntireFileFunc);


#define PLATFORM_CREATE_SEMAPHORE(name) void* name( int initialCount )
typedef PLATFORM_CREATE_SEMAPHORE(PlatformCreateSemaphoreFunc);

#define PLATFORM_DESTROY_SEMAPHORE(name) void name( void* handle )
typedef PLATFORM_DESTROY_SEMAPHORE(PlatformDestroySemaphoreFunc);

#define PLATFORM_WAIT_SEMAPHORE(name) void name( void* handle )
typedef PLATFORM_WAIT_SEMAPHORE(PlatformWaitSemaphoreFunc);

#define PLATFORM_SIGNAL_SEMAPHORE(name) void name( void* handle, int count )
typedef PLATFORM_SIGNAL_SEMAPHORE(PlatformSignalSemaphoreFunc);


#define PLATFORM_CURRENT_TIME_MILLIS(name) f64 name()
typedef PLATFORM_CURRENT_TIME_MILLIS(PlatformCurrentTimeMillisFunc);

#define PLATFORM_SHELL_EXECUTE(name) int name( char const* cmdLine )
typedef PLATFORM_SHELL_EXECUTE(PlatformShellExecuteFunc);

#define PLATFORM_PRINT(name) void name( const char *fmt, ... )
typedef PLATFORM_PRINT(PlatformPrintFunc);
#define PLATFORM_PRINT_VA(name) void name( const char *fmt, va_list args )
typedef PLATFORM_PRINT_VA(PlatformPrintVAFunc);


// TODO Shouldn't this be defined by the application really?
struct PlatformAPI
{
    // Memory
    PlatformAllocFunc*             Alloc;
    PlatformFreeFunc*              Free;

    // Context
    PlatformPushContextFunc*       PushContext;
    PlatformPopContextFunc*        PopContext;

    // Filesystem
#if 0
    PlatformGetAbsolutePathFunc*   GetAbsolutePath;
#endif

    PlatformReadEntireFileFunc*    ReadEntireFile;
    PlatformWriteEntireFileFunc*   WriteEntireFile;

    // Threading
    PlatformCreateSemaphoreFunc*   CreateSemaphore;
    PlatformDestroySemaphoreFunc*  DestroySemaphore;
    PlatformWaitSemaphoreFunc*     WaitSemaphore;
    PlatformSignalSemaphoreFunc*   SignalSemaphore;

    // Misc
    PlatformCurrentTimeMillisFunc* CurrentTimeMillis;
    PlatformShellExecuteFunc*      ShellExecute;

    PlatformPrintFunc*             Print;
    PlatformPrintFunc*             Error;
    PlatformPrintVAFunc*           PrintVA;
    PlatformPrintVAFunc*           ErrorVA;

#if 0
#if !CONFIG_RELEASE
    DebugPlatformFreeFileMemoryFunc* DEBUGFreeFileMemory;
    DebugPlatformListAllAssetsFunc* DEBUGListAllAssets;
    DebugPlatformJoinPathsFunc* DEBUGJoinPaths;
    DebugPlatformGetParentPathFunc* DEBUGGetParentPath;

    bool DEBUGquit;
#endif

    PlatformAddNewJobFunc* AddNewJob;
    PlatformCompleteAllJobsFunc* CompleteAllJobs;
    PlatformJobQueue* hiPriorityQueue;
    //PlatformJobQueue* loPriorityQueue;
    // NOTE Includes the main thread! (0)
    i32 coreThreadsCount;

    PlatformAllocateOrUpdateTextureFunc* AllocateOrUpdateTexture;
    PlatformDeallocateTextureFunc* DeallocateTexture;
#endif
};
extern PlatformAPI globalPlatform;


// TODO Add support for different log severities and categories/filters
// TODO Use the Context!
#define LOG globalPlatform.Print
#define LOGE globalPlatform.Error

