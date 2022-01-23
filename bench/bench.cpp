
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
#include "context.h"
#include "threading.h"
#include "datatypes.h"
#include "clock.h"
#include "logging.h"
#include "strings.h"

#include "platform.cpp"
#include "logging.cpp"
#include "win32_platform.cpp"

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

// Taken from https://rigtorp.se/spinlock/
struct SpinLockMutex
{
    atomic_bool lock;

    SpinLockMutex()
        : lock( false )
    {}

    void Lock()
    {
        for( ;; )
        {
            if( !lock.exchange( true, std::memory_order_acquire ) )
                break;

            while( lock.load( std::memory_order_relaxed ) )
                _mm_pause();
        }
    }

    void Unlock()
    {
        lock.store( false, std::memory_order_release );
    }
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

// TODO Would be interesting to give the threads a more 'realistic' workload, to see if the differences remain that big
TEST_MUTEX(Mutex);
TEST_MUTEX(RecursiveMutex);
TEST_MUTEX(PlatformMutex);

//TEST_MUTEX(Benaphore<PlatformSemaphore>);
TEST_MUTEX(Benaphore<PreshingSemaphore>);
TEST_MUTEX(Benaphore<Semaphore>);
//TEST_MUTEX(RecursiveBenaphore<PlatformSemaphore>);
TEST_MUTEX(RecursiveBenaphore<PreshingSemaphore>);
TEST_MUTEX(RecursiveBenaphore<Semaphore>);

TEST_MUTEX(SpinLockMutex);


int main(int argc, char** argv)
{
    SetUnhandledExceptionFilter( Win32::ExceptionHandler );
    Logging::ChannelDecl channels[] =
    {
        { "Platform" },
        { "Net" },
    };
    InitGlobalPlatform( channels );

    ::benchmark::Initialize(&argc, argv);
    if (::benchmark::ReportUnrecognizedArguments(argc, argv))
        return 1;
    ::benchmark::RunSpecifiedBenchmarks();
    ::benchmark::Shutdown();

    return 0;
}
