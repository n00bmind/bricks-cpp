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



struct Allocator;
struct Context;
template <typename T, typename AllocType = Allocator> struct Array;


namespace Platform
{

#define PLATFORM_ALLOC(x)               void* x( sz sizeBytes, u32 flags )
typedef PLATFORM_ALLOC(AllocFunc);
#define PLATFORM_FREE(x)                void x( void* memoryBlock )
typedef PLATFORM_FREE(FreeFunc);


#define PLATFORM_GET_CONTEXT(x)         Context** x()
typedef PLATFORM_GET_CONTEXT(GetContextFunc);
#define PLATFORM_PUSH_CONTEXT(x)        void x( Context const& newContext )
typedef PLATFORM_PUSH_CONTEXT(PushContextFunc);
#define PLATFORM_POP_CONTEXT(x)         void x()
typedef PLATFORM_POP_CONTEXT(PopContextFunc);


// TODO Do something better for paths
#define PLATFORM_PATH_MAX                  1024

#define PLATFORM_GET_ABSOLUTE_PATH(x)   bool x( char const* filename, char* outBuffer, sz outBufferLen )
typedef PLATFORM_GET_ABSOLUTE_PATH(GetAbsolutePathFunc);
// Returned buffer data must be null-terminated
#define PLATFORM_READ_ENTIRE_FILE(x)    Buffer<> x( char const* filename, Allocator* allocator )
typedef PLATFORM_READ_ENTIRE_FILE(ReadEntireFileFunc);
#define PLATFORM_WRITE_ENTIRE_FILE(x)   bool x( char const* filename, Array<Buffer<>> const& chunks, bool overwrite )
typedef PLATFORM_WRITE_ENTIRE_FILE(WriteEntireFileFunc);

    
    typedef void* ThreadHandle;

#define PLATFORM_THREAD_FUNC(x)         int x( void* userdata )
typedef PLATFORM_THREAD_FUNC(ThreadFunc);
#define PLATFORM_CREATE_THREAD(x)       Platform::ThreadHandle x( char const* name, Platform::ThreadFunc* threadFunc, void* userdata, Context const& threadContext )
typedef PLATFORM_CREATE_THREAD(CreateThreadFunc);
// Returns the thread's exit code
#define PLATFORM_JOIN_THREAD(x)         int x( Platform::ThreadHandle handle )
typedef PLATFORM_JOIN_THREAD(JoinThreadFunc);
#define PLATFORM_GET_THREAD_ID(x)       u32 x()
typedef PLATFORM_GET_THREAD_ID(GetThreadIdFunc);
#define PLATFORM_IS_MAIN_THREAD(x)      bool x()
typedef PLATFORM_IS_MAIN_THREAD(IsMainThreadFunc);

#define PLATFORM_CREATE_SEMAPHORE(x)    void* x( int initialCount )
typedef PLATFORM_CREATE_SEMAPHORE(CreateSemaphoreFunc);
#define PLATFORM_DESTROY_SEMAPHORE(x)   void x( void* handle )
typedef PLATFORM_DESTROY_SEMAPHORE(DestroySemaphoreFunc);
#define PLATFORM_WAIT_SEMAPHORE(x)      void x( void* handle )
typedef PLATFORM_WAIT_SEMAPHORE(WaitSemaphoreFunc);
#define PLATFORM_SIGNAL_SEMAPHORE(x)    void x( void* handle, int count )
typedef PLATFORM_SIGNAL_SEMAPHORE(SignalSemaphoreFunc);

#define PLATFORM_CREATE_MUTEX(x)        void* x()
typedef PLATFORM_CREATE_MUTEX(CreateMutexFunc);
#define PLATFORM_DESTROY_MUTEX(x)       void x( void* handle )
typedef PLATFORM_DESTROY_MUTEX(DestroyMutexFunc);
#define PLATFORM_LOCK_MUTEX(x)          void x( void* handle )
typedef PLATFORM_LOCK_MUTEX(LockMutexFunc);
#define PLATFORM_UNLOCK_MUTEX(x)        void x( void* handle )
typedef PLATFORM_UNLOCK_MUTEX(UnlockMutexFunc);


#define PLATFORM_ELAPSED_TIME_MILLIS(x) f64 x()
typedef PLATFORM_ELAPSED_TIME_MILLIS(ElapsedTimeMillisFunc);

#define PLATFORM_SHELL_EXECUTE(x)       int x( char const* cmdLine )
typedef PLATFORM_SHELL_EXECUTE(ShellExecuteFunc);

#define PLATFORM_TEST_CONNECTIVITY(x)   bool x()
typedef PLATFORM_TEST_CONNECTIVITY(TestConnectivityFunc);

#define PLATFORM_PRINT(x)               void x( const char *fmt, ... )
typedef PLATFORM_PRINT(PrintFunc);
#define PLATFORM_PRINT_VA(x)            void x( const char *fmt, va_list args )
typedef PLATFORM_PRINT_VA(PrintVAFunc);

} // namespace Platform


// TODO Shouldn't this be defined by the application really?
struct PlatformAPI
{
    // Memory
    Platform::AllocFunc*                        Alloc;
    Platform::FreeFunc*                         Free;

    // Context
    Platform::GetContextFunc*                   GetContext;
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
    Platform::GetThreadIdFunc*                  GetThreadId;
    Platform::IsMainThreadFunc*                 IsMainThread;

    Platform::CreateSemaphoreFunc*              CreateSemaphore;
    Platform::DestroySemaphoreFunc*             DestroySemaphore;
    Platform::WaitSemaphoreFunc*                WaitSemaphore;
    Platform::SignalSemaphoreFunc*              SignalSemaphore;
    Platform::CreateMutexFunc*                  CreateMutex;
    Platform::DestroyMutexFunc*                 DestroyMutex;
    Platform::LockMutexFunc*                    LockMutex;
    Platform::UnlockMutexFunc*                  UnlockMutex;

    // Misc
    Platform::ElapsedTimeMillisFunc*            ElapsedTimeMillis;
    Platform::ShellExecuteFunc*                 ShellExecute;
    Platform::TestConnectivityFunc*             TestConnectivity;

    Platform::PrintFunc*                        Print;
    Platform::PrintFunc*                        Error;
    Platform::PrintVAFunc*                      PrintVA;
    Platform::PrintVAFunc*                      ErrorVA;
    AssertHandlerFunc*                          DefaultAssertHandler;

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


namespace Platform
{
    void InitContextStack( Context const& baseContext );

    //void InitGlobalPlatform( PlatformAPI const& platformAPI, Buffer<Logging::ChannelDecl> logChannels );
    void TickGlobalPlatform();
}
