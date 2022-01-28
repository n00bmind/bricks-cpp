
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

