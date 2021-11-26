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

// NOTE These must be defined by the application
struct Context;
//struct AppState;


namespace Platform
{

#define PLATFORM_ALLOC(name)               void* name( sz sizeBytes, u32 flags )
typedef PLATFORM_ALLOC(AllocFunc);
#define PLATFORM_FREE(name)                void name( void* memoryBlock )
typedef PLATFORM_FREE(FreeFunc);


#define PLATFORM_PUSH_CONTEXT(name)        void name( Context const& newContext )
typedef PLATFORM_PUSH_CONTEXT(PushContextFunc);
#define PLATFORM_POP_CONTEXT(name)         void name()
typedef PLATFORM_POP_CONTEXT(PopContextFunc);


// TODO Do something better for paths
#define PLATFORM_PATH_MAX                  1024

#define PLATFORM_GET_ABSOLUTE_PATH(name)   bool name( char const* filename, char* outBuffer, sz outBufferLen )
typedef PLATFORM_GET_ABSOLUTE_PATH(GetAbsolutePathFunc);
// Returned buffer data must be null-terminated
#define PLATFORM_READ_ENTIRE_FILE(name)    Buffer<> name( char const* filename, Allocator* allocator )
typedef PLATFORM_READ_ENTIRE_FILE(ReadEntireFileFunc);
#define PLATFORM_WRITE_ENTIRE_FILE(name)   bool name( char const* filename, Array<Buffer<>> const& chunks, bool overwrite )
typedef PLATFORM_WRITE_ENTIRE_FILE(WriteEntireFileFunc);

    
    typedef void* ThreadHandle;

#define PLATFORM_THREAD_FUNC(name)         int name( void* userdata )
typedef PLATFORM_THREAD_FUNC(ThreadFunc);
#define PLATFORM_CREATE_THREAD(name)       Platform::ThreadHandle name( char const* name, Platform::ThreadFunc* threadFunc, void* userdata )
typedef PLATFORM_CREATE_THREAD(CreateThreadFunc);
// Returns the thread's exit code
#define PLATFORM_JOIN_THREAD(name)         int name( Platform::ThreadHandle handle )
typedef PLATFORM_JOIN_THREAD(JoinThreadFunc);

#define PLATFORM_CREATE_SEMAPHORE(name)    void* name( int initialCount )
typedef PLATFORM_CREATE_SEMAPHORE(CreateSemaphoreFunc);
#define PLATFORM_DESTROY_SEMAPHORE(name)   void name( void* handle )
typedef PLATFORM_DESTROY_SEMAPHORE(DestroySemaphoreFunc);
#define PLATFORM_WAIT_SEMAPHORE(name)      void name( void* handle )
typedef PLATFORM_WAIT_SEMAPHORE(WaitSemaphoreFunc);
#define PLATFORM_SIGNAL_SEMAPHORE(name)    void name( void* handle, int count )
typedef PLATFORM_SIGNAL_SEMAPHORE(SignalSemaphoreFunc);


#define PLATFORM_CURRENT_TIME_MILLIS(name) f64 name()
typedef PLATFORM_CURRENT_TIME_MILLIS(CurrentTimeMillisFunc);

#define PLATFORM_SHELL_EXECUTE(name)       int name( char const* cmdLine )
typedef PLATFORM_SHELL_EXECUTE(ShellExecuteFunc);

#define PLATFORM_PRINT(name)               void name( const char *fmt, ... )
typedef PLATFORM_PRINT(PrintFunc);
#define PLATFORM_PRINT_VA(name)            void name( const char *fmt, va_list args )
typedef PLATFORM_PRINT_VA(PrintVAFunc);

} // namespace Platform


// TODO Shouldn't this be defined by the application really?
struct PlatformAPI
{
    // Memory
    Platform::AllocFunc*                        Alloc;
    Platform::FreeFunc*                         Free;

    // Context
    Platform::PushContextFunc*                  PushContext;
    Platform::PopContextFunc*                   PopContext;

    // Filesystem
#if 0
    Platform::GetAbsolutePathFunc*              GetAbsolutePath;
#endif

    Platform::ReadEntireFileFunc*               ReadEntireFile;
    Platform::WriteEntireFileFunc*              WriteEntireFile;

    // Threading
    Platform::CreateThreadFunc*                 CreateThread;
    Platform::JoinThreadFunc*                   JoinThread;
    Platform::CreateSemaphoreFunc*              CreateSemaphore;
    Platform::DestroySemaphoreFunc*             DestroySemaphore;
    Platform::WaitSemaphoreFunc*                WaitSemaphore;
    Platform::SignalSemaphoreFunc*              SignalSemaphore;

    // Misc
    Platform::CurrentTimeMillisFunc*            CurrentTimeMillis;
    Platform::ShellExecuteFunc*                 ShellExecute;

    Platform::PrintFunc*                        Print;
    Platform::PrintFunc*                        Error;
    Platform::PrintVAFunc*                      PrintVA;
    Platform::PrintVAFunc*                      ErrorVA;

#if 0
#if !CONFIG_RELEASE
    Platform::DebugFreeFileMemoryFunc*          DEBUGFreeFileMemory;
    Platform::DebugListAllAssetsFunc*           DEBUGListAllAssets;
    Platform::DebugJoinPathsFunc*               DEBUGJoinPaths;
    Platform::DebugGetParentPathFunc*           DEBUGGetParentPath;

    bool DEBUGquit;
#endif

    Platform::AddNewJobFunc*                    AddNewJob;
    Platform::CompleteAllJobsFunc*              CompleteAllJobs;
    Platform::JobQueue*                         hiPriorityQueue;
    //JobQueue* loPriorityQueue;
    // NOTE Includes the main thread! (0)
    i32 coreThreadsCount;

    Platform::AllocateOrUpdateTextureFunc*      AllocateOrUpdateTexture;
    Platform::DeallocateTextureFunc*            DeallocateTexture;
#endif
};
extern PlatformAPI globalPlatform;


// TODO Add support for different log severities and categories/filters
// TODO Use the Context!
#define LOG globalPlatform.Print
#define LOGW globalPlatform.Error
#define LOGE globalPlatform.Error

