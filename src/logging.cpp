
namespace Logging
{

    void LogInternalVA( char const* channelName, Volume volume, char const* file, int line, char const* msg, va_list args )
    {
        State* state = CTX.logState;

        ASSERT( !state->channels.Empty(), "No channels.. have you called Logging::Init?" );
        // TODO We probably won't need this. Just check we're initialized?
        Channel const* channel = state->channels.Get( channelName );
        ASSERT( channelName && channel, "Invalid log channel" );
        if( !channel )
            return;

        if( volume < channel->minVolume )
            return;

        // Figure out total length of the msg + entry header
        int len         = vsnprintf( nullptr, 0, msg, args );
        // TODO Formalize what we're trying to do here using a BipBuffer instead .. https://www.codeproject.com/Articles/3479/The-Bip-Buffer-The-Circular-Buffer-with-a-Twist
        // AND remove all 'Adjacent' variations from RingBuffer(s) cause they're quirky
        char* msgBuffer = state->msgBuffer.PushEmptyRangeAdjacent( len + 1, false );

        Entry* newEntry = state->entryQueue.PushEmpty();
        newEntry->channelName = channelName;
        newEntry->sourceFile  = file;
        newEntry->sourceLine  = line;
        newEntry->timeSeconds = Core::AppTimeSeconds();
        newEntry->threadId    = Core::GetThreadId();
        newEntry->volume      = volume;
        newEntry->msgLen      = len;
        newEntry->msg         = msgBuffer;
        
        // Actually write the msg string
        vsnprintf( msgBuffer, (size_t)(len + 1), msg, args );
        // May not be necessary
        msgBuffer[len] = 0;

        state->entrySemaphore.Signal();
    }
    void LogInternal( char const* channelName, Volume volume, char const* file, int line, char const* msg, ... )
    {
        va_list args;
        va_start( args, msg );

        LogInternalVA( channelName, volume, file, line, msg, args );
        va_end( args );
    }


    namespace Endpoints
    {
        LOG_ENDPOINT(DebugLog)
        {
            Entry const& e = entry;
            if( e.volume < Volume::Error )
                globalPlatform.Print( "%s: %.3f %s : %s\n", e.volume.Name(), e.timeSeconds, e.channelName, e.msg );
            else
                globalPlatform.Error( "%s: %.3f %s : %s\n", e.volume.Name(), e.timeSeconds, e.channelName, e.msg );
        }

        // TODO 
        LOG_ENDPOINT(RawFileLog);
    }

    void AttachEndpoint( StaticStringHash name, EndpointFunc* endpoint, void* userdata /*= nullptr*/ )
    {
        // FIXME Sync!

        u32 id = name.Hash32();

        EndpointInfo* match = nullptr;
        for( EndpointInfo& e : CTX.logState->endpoints )
            if( id == e.id && name == e.name )
            {
                e = { name.data, endpoint, userdata, id };
                match = &e;
                break;
            }

        if( !match )
            CTX.logState->endpoints.Push( { name.data, endpoint, userdata, id } );
    }


    PLATFORM_THREAD_FUNC(LoggingThread)
    {
        State* state = (State*)userdata;

        while( state->thread.LOAD_RELAXED() )
        {
            state->entrySemaphore.Wait();

            // The only situation where this will fail is when the application is shutting down
            Entry entry;
            if( state->entryQueue.TryPop( &entry ) )
            {
                // Send to available endpoints
                for( EndpointInfo const& tgt : state->endpoints )
                    tgt.func( entry, tgt.userdata );

                // Clear out the range in the msg buffer, and advance it as much as we can
                ZEROP( (void*)entry.msg, entry.msgLen );

                // Remove any zero'd out chars currently present in the msgBuffer, to account for the fact that entries and msg strings
                // have been pushed in two stages and hence could be out of order with respect to each other
                // (Since we're the only thread consuming stuff, we're safe doing whatever at the tail end of the buffer)
                while( !state->msgBuffer.Empty() )
                {
                    char* c = &state->msgBuffer.Tail();
                    if( *c )
                        break;
                    state->msgBuffer.TryPop( c );
                }
            }
        }

        return 0;
    }


    void Init( State* state, Buffer<ChannelDecl> channels )
    {
        // Init everything from the main thread's arena
        INIT( state->channels )( I32(channels.length) );
        INIT( state->endpoints )( 8 );
        INIT( state->msgBuffer )( 64 * 1024 );
        INIT( state->entryQueue )( 1024 );
        INIT( state->entrySemaphore );
        INIT( state->thread )( nullptr );

        InitArena( &state->threadArena );
        InitArena( &state->threadTmpArena );

        for( ChannelDecl const& cd : channels )
        {
            Channel* c = state->channels.PutEmpty( cd.name );
            c->minVolume = cd.minVolume;
        }

        // Once ready, set the state in this thread's Context so it's ready to use
        CTX.logState = state;

        // DefaultEndpoints
        AttachEndpoint( "StandardOut", Endpoints::DebugLog );
        // TODO File creation etc
        //AttachEndpoint( "FileOut", Endpoints::RawFileLog );

        // Set up an initial context for the thread
        Context threadContext = InitContext( &state->threadArena, &state->threadTmpArena, state );
        state->thread.STORE_RELAXED( Core::CreateThread( "LoggingThread", LoggingThread, state, threadContext ) );
    }

    void Shutdown( State* state )
    {
        Platform::ThreadHandle thread = state->thread.LOAD_RELAXED();
        state->thread.STORE_RELAXED( nullptr );
        state->entrySemaphore.Signal();

        Core::JoinThread( thread );
    }
} // namespace Logging
