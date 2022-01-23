
namespace Logging
{

    void LogInternal( char const* channelName, Volume volume, char const* file, int line, char const* msg, va_list args )
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
        char* msgBuffer = state->msgBuffer.PushEmptyRangeAdjacent( len + 1, false );

        Entry* newEntry = state->entryQueue.PushEmpty();
        newEntry->channelName = channelName;
        newEntry->sourceFile  = file;
        newEntry->sourceLine  = line;
        newEntry->timestamp   = Core::AppTimeSeconds();
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

        LogInternal( channelName, volume, file, line, msg, args );
        va_end( args );
    }


    namespace Endpoints
    {
        LOG_ENDPOINT(DebugLog)
        {
            if( e.volume < Volume::Error )
                globalPlatform.Print( "%s: %.3f %s : %s\n", e.volume.Name(), e.timestamp, e.channelName, e.msg );
            else
                globalPlatform.Error( "%s: %.3f %s : %s\n", e.volume.Name(), e.timestamp, e.channelName, e.msg );
        }

        // TODO 
        LOG_ENDPOINT(RawFileLog);
    }

    PLATFORM_THREAD_FUNC(LoggingThread)
    {
        State* state = (State*)userdata;

        // Set up an initial context for the thread
        Core::SetUpThreadContext( &state->threadArena,
                                  &state->threadTmpArena,
                                  GetGlobalLoggingState() );

        while( state->thread.LOAD_RELAXED() )
        {
            state->entrySemaphore.Wait();

            // The only situation where this will fail is when the application is shutting down
            Entry entry;
            if( state->entryQueue.TryPop( &entry ) )
            {
                // Send to available endpoints
                for( EndpointFunc* tgt : state->endpoints )
                    tgt( entry );

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


    void AttachEndpoint( EndpointFunc* endpoint, State* state )
    {
        state->endpoints.Push( endpoint );
    }

    void InitThreadContext( State* state )
    {
        CTX.logState = state;
    }

    void Init( State* state, Buffer<ChannelDecl> channels )
    {
        INIT( state->channels )( I32(channels.length) );
        INIT( state->endpoints )( 8 );
        INIT( state->msgBuffer )( 64 * 1024 );
        INIT( state->entryQueue )( 1024 );
        INIT( state->entrySemaphore );
        INIT( state->thread )( nullptr );

        for( ChannelDecl const& cd : channels )
        {
            Channel* c = state->channels.PutEmpty( cd.name );
            c->minVolume = cd.minVolume;
        }

        // DefaultEndpoints
        AttachEndpoint( Endpoints::DebugLog, state );
        // TODO File creation etc
        //AttachEndpoint( Endpoints::RawFileLog, state );

        state->thread.STORE_RELAXED( Core::CreateThread( "LoggingThread", LoggingThread, state ) );
        InitThreadContext( state );
    }

    void Shutdown( State* state )
    {
        Platform::ThreadHandle thread = state->thread.LOAD_RELAXED();
        state->thread.STORE_RELAXED( nullptr );
        state->entrySemaphore.Signal();

        Core::JoinThread( thread );
    }
} // namespace Logging
