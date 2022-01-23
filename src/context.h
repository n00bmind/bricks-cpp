#pragma once

namespace Logging
{
    struct State;
}

// Global context to use across the entire application
// NOTE We wanna make sure everything inside here is not generally synchronized across threads to keep it fast!
// i.e. allocator, temp allocator, assert handler, logger (how do we collate and flush all logs?)
struct Context
{
    Allocator       allocator;
    Allocator       tmpAllocator;

    Logging::State* logState;

    // ...
    // TODO Application-defined custom data
};
void InitContextStack( Context const& baseContext );

extern thread_local Context** globalContext;
#define CTX (**globalContext)
#define CTX_ALLOC &CTX.allocator
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

