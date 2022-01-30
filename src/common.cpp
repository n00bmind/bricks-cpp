//
// NOTE NOTE NOTE Include this both on platform exe & the application dll
//

// TODO Use this to also #include the bare minimal files required on both the platform & app


// NOTE Since this is only defined on the platform side, it needs to be passed manually to the app DLL
PlatformAPI globalPlatform;


// Global Context

Context& GetContext()
{
    // Cache the pointer so we dont have to call through to the platform each time
    persistent thread_local Context** globalContext = nullptr;
    if( !globalContext )
        globalContext = globalPlatform.GetContext();

    return **globalContext;
}

Context InitContext( MemoryArena* mainArena, MemoryArena* tmpArena, Logging::State* logState )
{
    Context ctx = {};
    ctx.allocator = Allocator::CreateFrom( mainArena );
    ctx.tmpAllocator = Allocator::CreateFrom( tmpArena );
    ctx.logState = logState;

    return ctx;
}


// Global assert handler

internal AssertHandlerFunc* globalAssertHandler = nullptr;

AssertHandlerFunc* GetGlobalAssertHandler()
{
    return globalAssertHandler ? globalAssertHandler : globalPlatform.DefaultAssertHandler;
}

void SetGlobalAssertHandler( AssertHandlerFunc* f )
{
    globalAssertHandler = f;
}
