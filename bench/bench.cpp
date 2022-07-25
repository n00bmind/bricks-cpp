
#include <mutex>
#include "win32.h"
#include <wininet.h>
#include <DbgHelp.h>

#include "benchmark/benchmark.h"

#pragma comment(lib, "wininet")
#if CONFIG_DEBUG
#pragma comment(lib, "Debug/benchmark.lib")
#else
#pragma comment(lib, "Release/benchmark.lib")
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
#include "strings.h"
#include "logging.h"
#include "serialization.h"
#include "serialize_binary.h"

#include "../test/test.h"

#include "common.cpp"
#include "logging.cpp"
#include "platform.cpp"
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
    i64 sharedValue;

    MutexTester( int threadCount_, int iterationCount_ )
        : iterationCount( iterationCount_ )
        , threadCount( threadCount_ )
        , sharedValue( 0 )
    {}

    bool Test()
    {
        sharedValue = 0;

        Array<Platform::ThreadHandle> threads( threadCount );
        for (int i = 0; i < threadCount; i++)
            threads.Push( Core::CreateThread( "Test thread", MutexTesterThread<T>, this ) );
        for( Platform::ThreadHandle& t : threads )
            Core::JoinThread( t );

        return( sharedValue == iterationCount );
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
        DoNotOptimize( tester->sharedValue++ );
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

#if 0
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
#endif


static void TestBinarySerializer( benchmark::State& state )
{
    BucketArray<u8> buffer( 2048 * 1024, CTX_TMPALLOC );
    BinaryWriter w( &buffer );

    SerialTypeComplex cmp = { { 42 }, {}, "Hello sailor" };
    INIT( cmp.nums )( { 0, 1, 2, 3, 4, 5, 6, 7 } );
    SerialTypeDeeper deeper = { { cmp, 666  }, "Apartense vacas, que la vida es corta" };

    SerialTypeChunky before;
    before.deeper.Reset( 8000 );
    for( int i = 0; i < before.deeper.capacity; ++i )
        before.deeper.Push( deeper );


    for( auto _ : state )
    {
        buffer.Clear();
        Reflect( w, before );

        BinaryReader r( &buffer );
        SerialTypeChunky after;
        Reflect( r, after );
    }
}

BENCHMARK(TestBinarySerializer)
    ->Unit(benchmark::kMicrosecond)
    //->Iterations(1)
    ->MeasureProcessCPUTime();


int main(int argc, char** argv)
{
    SetUnhandledExceptionFilter( Win32::ExceptionHandler );
    Logging::ChannelDecl channels[] =
    {
        { "Platform" },
        { "Net" },
    };
    Win32::InitGlobalPlatform( channels );

    ::benchmark::Initialize(&argc, argv);
    if (::benchmark::ReportUnrecognizedArguments(argc, argv))
        return 1;
    ::benchmark::RunSpecifiedBenchmarks();
    ::benchmark::Shutdown();

    return 0;
}
