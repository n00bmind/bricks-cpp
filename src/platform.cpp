
//
// Include this only on platform exe, not the application dll
//


// Global stack of Contexts
thread_local Context   globalContextStack[64];
thread_local Context*  globalContextPtr;
thread_local Context** globalContext;


void InitContextStack( Context const& baseContext )
{
    globalContextPtr  = globalContextStack;
    *globalContextPtr = baseContext;
    globalContext     = &globalContextPtr;
}

PLATFORM_PUSH_CONTEXT( PushContext )
{
    globalContextPtr++;
    ASSERT( globalContextPtr < globalContextStack + ARRAYCOUNT(globalContextStack) );
    *globalContextPtr = newContext;
}

PLATFORM_POP_CONTEXT( PopContext )
{
    if( globalContextPtr > globalContextStack )
        globalContextPtr--;
}

namespace Core
{
    void SetUpThreadContext( MemoryArena* mainArena, MemoryArena* tmpArena, Logging::State* logState )
    {
        // Only do this at the beginning of new threads
        ASSERT( !globalContext );

        if( !IsInitialized( *mainArena ) )
            InitArena( mainArena );
        if( !IsInitialized( *tmpArena ) )
            InitArena( tmpArena );

        Context threadContext =
        {
            Allocator::CreateFrom( mainArena ),
            Allocator::CreateFrom( tmpArena ),
        };
        InitContextStack( threadContext );

        Logging::InitThreadContext( logState );
    }
}


// Global platform

PlatformAPI globalPlatform;

internal MemoryArena globalPlatformArena;
internal MemoryArena globalTmpArena;


Logging::State* GetGlobalLoggingState()
{
    // NOTE Merely declaring this (or any data type in datatypes.h) as a global requires
    // a properly setup Context, so we may want to initialize that lazily instead?
    persistent Logging::State logState;
    return &logState;
}

void InitGlobalPlatform( PlatformAPI const& platformAPI, Buffer<Logging::ChannelDecl> logChannels )
{
    globalPlatform = platformAPI;

    Core::SetUpThreadContext( &globalPlatformArena, &globalTmpArena, nullptr );

    // Set up initial logging state (this requires a working Context & allocators)
    Logging::Init( GetGlobalLoggingState(), logChannels );
}

