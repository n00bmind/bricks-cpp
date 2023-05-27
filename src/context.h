#pragma once

struct MemoryArena;
namespace Logging
{
    struct State;
}

// Global context to use across the entire application
// NOTE We wanna make sure everything inside here is not generally synchronized across threads to keep it fast!
struct Context
{
    Allocator       allocator;
    Allocator       tmpAllocator;

    Logging::State* logState;

    // ...
    // TODO Application-defined custom data
};
Context InitContext( MemoryArena* mainArena, MemoryArena* tmpArena, Logging::State* logState );

// Access the platform's thread-local Context from anywhere in the app
Context& GetContext();
#define CTX          ::GetContext()
#define CTX_ALLOC    &CTX.allocator
#define CTX_TMPALLOC &CTX.tmpAllocator


struct ScopedContext
{
    ScopedContext( Context const& newContext )
    { globalPlatform.PushContext( newContext ); }
    ~ScopedContext()
    { globalPlatform.PopContext(); }
};
#define WITH_CONTEXT(context) \
    ScopedContext ctx_##__FUNC__##__LINE__(context)

