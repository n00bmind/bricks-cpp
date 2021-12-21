
#include <mutex>
#include "win32.h"
#include <DbgHelp.h>

#include "benchmark/benchmark.h"

#if CONFIG_RELEASE
#pragma comment(lib, "Release/benchmark.lib")
#else
// NOTE 'Dev' config will use the debug benchmark lib
#pragma comment(lib, "Debug/benchmark.lib")
#endif

#include "magic.h"
#include "common.h"
#include "intrinsics.h"
#include "maths.h"
#include "platform.h"
#include "memory.h"
#include "threading.h"
#include "datatypes.h"
#include "strings.h"

#include "platform.cpp"
#include "win32_platform.cpp"

PlatformAPI globalPlatform;

// Conflicts with benchmark.h
#undef internal

using namespace benchmark;


template <typename T>
PLATFORM_THREAD_FUNC(MutexTesterThread);
template <typename T>
struct MutexTester
{
    T mutex;
    const int iterationCount;
    const int threadCount;
    i64 value;

    MutexTester( int threadCount_, int iterationCount_ )
        : iterationCount( iterationCount_ )
        , threadCount( threadCount_ )
        , value( 0 )
    {}

    bool Test()
    {
        value = 0;

        Array<Platform::ThreadHandle> threads( threadCount );
        for (int i = 0; i < threadCount; i++)
            threads.Push( Core::CreateThread( "Test thread", MutexTesterThread<T>, this ) );
        for( Platform::ThreadHandle& t : threads )
            Core::JoinThread( t );

        return( value == iterationCount );
    }
};
template <typename T>
PLATFORM_THREAD_FUNC(MutexTesterThread)
{
    MutexTester<T>* tester = (MutexTester<T>*)userdata;

    int threadIterationCount = tester->iterationCount / tester->threadCount;
    ASSERT( threadIterationCount * tester->threadCount == tester->iterationCount );

    for( int i = 0; i < threadIterationCount; i++ )
    {
        tester->mutex.Lock();
        DoNotOptimize( tester->value++ );
        ClobberMemory();
        tester->mutex.Unlock();
    }
    return 0;
}

struct NoMutex
{
    // TODO Not super clear how more 'lightweight' stuff (like CriticalSection) compares with mutex in recent times
    std::mutex m;

    void Lock()     { /*m.lock();*/ }
    void Unlock()   { /*m.unlock();*/ }
};


template <typename T>
static void TestMutex( benchmark::State& state )
{
    MutexTester<T> mutex( 4, 1000000 );
    for( auto _ : state )
    {
        bool result = mutex.Test();
        if( !result )
        {
            state.SkipWithError( "INCORRECT RESULT!" );
            break;
        }
    }
}

#define TEST_MUTEX(T)                   \
    BENCHMARK_TEMPLATE(TestMutex, T)    \
        ->Unit(benchmark::kMillisecond) \
        ->MeasureProcessCPUTime();

// TODO 
// TODO Check the assembly to ensure this is actually doing work!
TEST_MUTEX(StdMutex);
//TEST_MUTEX(NonRecursiveMutex<PlatformSemaphore>);
TEST_MUTEX(NonRecursiveMutex<Semaphore>);
TEST_MUTEX(NonRecursiveMutex<StdSemaphore>);
//TEST_MUTEX(Mutex<PlatformSemaphore>);
TEST_MUTEX(Mutex<Semaphore>);
TEST_MUTEX(Mutex<StdSemaphore>);

TEST_MUTEX(NoMutex);

int main(int argc, char** argv)
{
    SetUnhandledExceptionFilter( Win32::ExceptionHandler );
    InitGlobalPlatform();

    ::benchmark::Initialize(&argc, argv);
    if (::benchmark::ReportUnrecognizedArguments(argc, argv))
        return 1;
    ::benchmark::RunSpecifiedBenchmarks();
    ::benchmark::Shutdown();

    return 0;
}
