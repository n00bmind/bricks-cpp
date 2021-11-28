#pragma once

// TODO Take the ideas from https://preshing.com/20150316/semaphores-are-surprisingly-versatile/ to
// implement the most useful primitives using the same underlying platform semaphore primitive


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

    void Singal( int count = 1 )
    {
        globalPlatform.SignalSemaphore( handle, count );
    }
};


/////     MUTEX     /////

struct Mutex
{
    // TODO Not super clear how more 'lightweight' stuff (like CriticalSection) compares with mutex in recent times
    // TODO Implement the 'Benaphore' from https://github.com/preshing/cpp11-on-multicore/blob/master/common/benaphore.h
    // TODO Create some synthetic benchmarks for comparison
    std::mutex m;

    void Lock()     { m.lock(); }
    void Unlock()   { m.unlock(); }

    struct ScopedLock
    {
        Mutex& m;

        ScopedLock( Mutex& m_ ) : m( m_ ) { m.Lock(); }
        ~ScopedLock()                     { m.Unlock(); }
    };
};


