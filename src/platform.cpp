//
// NOTE NOTE NOTE Include this only on platform exe, not the application dll
//

namespace Platform
{

// Global stack of Contexts
thread_local Context   globalContextStack[64];
thread_local Context*  globalContextPtr;

void InitContextStack( Context const& baseContext )
{
    globalContextPtr  = globalContextStack;
    *globalContextPtr = baseContext;
}


PLATFORM_GET_CONTEXT( GetContext )
{
    return &globalContextPtr;
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


// Global platform

internal MemoryArena globalPlatformArena;
internal MemoryArena globalTmpArena;


internal Logging::State* GetGlobalLoggingState()
{
    // NOTE Merely declaring this (or any data type in datatypes.h) as a global requires
    // a properly setup Context, so just create the object lazily
    persistent Logging::State logState;
    return &logState;
}

void InitGlobalPlatform( Platform::API const& platformAPI, Buffer<Logging::ChannelDecl> logChannels )
{
    globalPlatform = platformAPI;

    InitArena( &globalPlatformArena );
    InitArena( &globalTmpArena );

    // Set up Context for the main thread
    Context threadContext =
    {
        Allocator::CreateFrom( &globalPlatformArena ),
        Allocator::CreateFrom( &globalTmpArena ),
    };
    InitContextStack( threadContext );

    // Set up initial logging state (this requires a working Context & allocators)
    Logging::Init( GetGlobalLoggingState(), logChannels );
}

// Mainly for hot-reloading support
void RestartGlobalPlatform()
{
    Logging::ResetEndpoints( GetGlobalLoggingState() );
}

void TickGlobalPlatform()
{
    ClearArena( &globalTmpArena );
}

} // namespace Platform

