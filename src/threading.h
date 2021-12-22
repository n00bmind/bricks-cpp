#pragma once


namespace Core
{
    inline Platform::ThreadHandle CreateThread( char const* name, Platform::ThreadFunc threadFunc, void* userdata = nullptr )
    {
        return globalPlatform.CreateThread( name, threadFunc, userdata );
    }

    inline int JoinThread( Platform::ThreadHandle handle )
    {
        return globalPlatform.JoinThread( handle );
    }
} // namespace Core




/////     SEMAPHORE     /////

// Taken from https://stackoverflow.com/a/19659736/2151254 & https://elweitzel.de/drupal/?q=node/6
struct Semaphore
{
private:
    std::mutex mutex;
    std::condition_variable condition;
    i32 count;

public:
    Semaphore( i32 count_ = 0 )
        : count( count_ )
    {}

    void Signal( i32 count_ = 1 )
    {
        std::unique_lock<std::mutex> lock( mutex );

        count += count_;
        for( int i = 0; i < count_; ++i )
            condition.notify_one();
    }

    void Wait()
    {
        std::unique_lock<std::mutex> lock( mutex );

        // Handle spurious wake-ups
        condition.wait( lock, [&]{ return count > 0; } );
        count--;
    }

    bool Wait( int millis )
    {
        auto timeout = std::chrono::milliseconds( millis );

        std::unique_lock<std::mutex> lock( mutex );
        if( !condition.wait_for( lock, timeout, [&]{ return count > 0; } ) )
            return false;

        --count;
        return true;
    }

    bool TryWait()
    {
        std::unique_lock<std::mutex> lock( mutex );

        if( count > 0 )
        {
            --count;
            return true;
        }
        return false;
    }
};


// TODO See https://github.com/preshing/cpp11-on-multicore/blob/master/common/sema.h for OSX & linux implementations
struct PlatformSemaphore
{
private:
    void* handle;

    // Disallow copying
    PlatformSemaphore( const PlatformSemaphore& other ) = delete;
    PlatformSemaphore& operator=( const PlatformSemaphore& other ) = delete;

public:

    PlatformSemaphore( int initialCount = 0 )
    {
        ASSERT( initialCount >= 0 );
        handle = globalPlatform.CreateSemaphore( initialCount );
    }
    
    ~PlatformSemaphore()
    {
        globalPlatform.DestroySemaphore( handle );
    }

    void Wait()
    {
        globalPlatform.WaitSemaphore( handle );
    }

    void Signal( int count = 1 )
    {
        globalPlatform.SignalSemaphore( handle, count );
    }
};


// From https://github.com/preshing/cpp11-on-multicore/blob/master/common/sema.h (LightweightSemaphore)
class PreshingSemaphore
{
private:
    PlatformSemaphore semaphore;
    atomic_i32 signalCount;

    void SpinAndWait()
    {
        int oldCount;
        // Is there a better way to set the initial spin count?
        // If we lower it to 1000, testBenaphore becomes 15x slower on my Core i7-5930K Windows PC,
        // as threads start hitting the kernel semaphore.
        int spin = 10000;
        while( spin-- )
        {
            oldCount = signalCount.load( std::memory_order_relaxed );
            if( oldCount > 0 && signalCount.compare_exchange_strong( oldCount, oldCount - 1, std::memory_order_acquire ) )
                return;
            std::atomic_signal_fence( std::memory_order_acquire );     // Prevent the compiler from collapsing the loop.
        }
        oldCount = signalCount.fetch_sub( 1, std::memory_order_acquire );
        if (oldCount <= 0)
        {
            semaphore.Wait();
        }
    }

public:
    PreshingSemaphore( int initialCount = 0 )
        : signalCount( initialCount )
    {
        ASSERT( initialCount >= 0 );
    }

    bool TryWait()
    {
        int oldCount = signalCount.load( std::memory_order_relaxed );
        return oldCount > 0
            && signalCount.compare_exchange_strong( oldCount, oldCount - 1, std::memory_order_acquire );
    }

    void Wait()
    {
        if( !TryWait() )
            SpinAndWait();
    }

    void Signal( int count = 1 )
    {
        ASSERT( count > 0 );
        int oldCount = signalCount.fetch_add( count, std::memory_order_release );
        int toRelease = -oldCount < count ? -oldCount : count;
        if( toRelease > 0 )
        {
            semaphore.Signal( toRelease );
        }
    }
};



/////     MUTEX     /////

struct Mutex
{
    std::mutex m;

    void Lock()     { m.lock(); }
    bool TryLock()  { return m.try_lock(); }
    void Unlock()   { m.unlock(); }

    struct Scope
    {
        Mutex& m;

        Scope( Mutex& m_ ) : m( m_ )
        { m.Lock(); }

        ~Scope()
        { m.Unlock(); }
    };
};

struct RecursiveMutex
{
    std::recursive_mutex m;

    void Lock()     { m.lock(); }
    bool TryLock()  { return m.try_lock(); }
    void Unlock()   { m.unlock(); }

    struct Scope
    {
        RecursiveMutex& m;

        Scope( RecursiveMutex& m_ ) : m( m_ )
        { m.Lock(); }

        ~Scope()
        { m.Unlock(); }
    };
};


// NOTE Uses CriticalSection in Win32
struct PlatformMutex
{
    void* mutex;

    PlatformMutex()
    {
        mutex = globalPlatform.CreateMutex();
    }
    ~PlatformMutex()
    {
        globalPlatform.DestroyMutex( mutex );
    }

    void Lock()
    {
        globalPlatform.LockMutex( mutex );
    }
    void Unlock()
    {
        globalPlatform.UnlockMutex( mutex );
    }
};


// From https://github.com/preshing/cpp11-on-multicore/blob/master/common/benaphore.h (NonRecursiveBenaphore)
template <typename SemaphoreType>
struct Benaphore
{
private:
    SemaphoreType semaphore;
    atomic_i32 contention;

public:
    Benaphore()
        : contention(0)
    {}

    void Lock()
    {
        // fetch_add returns the previous value
        if( contention.fetch_add( 1, std::memory_order_acquire ) > 0 )
        {
            semaphore.Wait();
        }
    }

    bool TryLock()
    {
        int expected = contention.load( std::memory_order_relaxed );
        if( expected != 0 )
            return false;
        return contention.compare_exchange_strong( expected, 1, std::memory_order_acquire );
    }

    void Unlock()
    {
        int oldCount = contention.fetch_sub( 1, std::memory_order_release );
        ASSERT( oldCount > 0 );
        if( oldCount > 1 )
        {
            semaphore.Signal();
        }
    }
};

// From https://github.com/preshing/cpp11-on-multicore/blob/master/common/benaphore.h (RecursiveBenaphore)
template <typename SemaphoreType>
struct RecursiveBenaphore
{
private:
    SemaphoreType semaphore;
    // TODO Does this work for non-std threads?
    std::atomic<std::thread::id> owner;
    atomic_i32 contention;
    int recursion;

public:
    RecursiveBenaphore()
        : owner( std::thread::id() )
        , contention( 0 )
        , recursion( 0 )
    {
        // If this assert fails on your system, you'll have to replace std::thread::id with a
        // more compact, platform-specific thread ID, or just comment the assert and live with
        // the extra overhead.
        ASSERT( owner.is_lock_free() );
    }

    void Lock()
    {
        std::thread::id tid = std::this_thread::get_id();
        if( contention.fetch_add( 1, std::memory_order_acquire ) > 0 )
        {
            if( tid != owner.load( std::memory_order_relaxed ) )
                semaphore.Wait();
        }
        //--- We are now inside the lock ---
        owner.store( tid, std::memory_order_relaxed );
        recursion++;
    }

    bool TryLock()
    {
        std::thread::id tid = std::this_thread::get_id();
        if( owner.load( std::memory_order_relaxed ) == tid )
        {
            // Already inside the lock
            contention.fetch_add( 1, std::memory_order_relaxed );
        }
        else
        {
            int expected = contention.load( std::memory_order_relaxed );
            if( expected != 0 )
                return false;
            if( !contention.compare_exchange_strong( expected, 1, std::memory_order_acquire ) )
                return false;
            //--- We are now inside the lock ---
            owner.store( tid, std::memory_order_relaxed );
        }

        recursion++;
        return true;
    }

    void Unlock()
    {
        ASSERT( std::this_thread::get_id() == owner.load( std::memory_order_relaxed ) );

        recursion--;
        if( recursion == 0 )
            owner.store( std::thread::id(), std::memory_order_relaxed );

        if( contention.fetch_sub( 1, std::memory_order_release ) > 1 )
        {
            if( recursion == 0 )
                semaphore.Signal();
        }
        //--- We are now outside the lock ---
    }
};


// TODO Implement https://github.com/preshing/cpp11-on-multicore/blob/master/common/rwlock.h
// TODO Implement Event from https://elweitzel.de/drupal/?q=node/6
// Also check AutoResetEvent in https://preshing.com/20150316/semaphores-are-surprisingly-versatile/

