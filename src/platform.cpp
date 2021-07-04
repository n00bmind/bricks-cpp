
//
// Include this only on platform exe, not the application dll
//


// Global stack of Contexts
thread_local Context   globalContextStack[64];
thread_local Context*  globalContextPtr;


inline void InitContextStack( Context const& baseContext )
{
    globalContextPtr  = globalContextStack;
    *globalContextPtr = baseContext;
    globalContext     = &globalContextPtr;
}

inline PLATFORM_PUSH_CONTEXT( PushContext )
{
    globalContextPtr++;
    ASSERT( globalContextPtr < globalContextStack + ARRAYCOUNT(globalContextStack) );
    *globalContextPtr = newContext;
}

inline PLATFORM_POP_CONTEXT( PopContext )
{
    if( globalContextPtr > globalContextStack )
        globalContextPtr--;
}
