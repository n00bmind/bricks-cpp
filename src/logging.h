#pragma once

namespace Logging
{
#define VOLUME_ITEMS(x) \
    x(Debug,   "DEBUG") \
    x(Info,    "INFO ") \
    x(Warning, "WARN ") \
    x(Error,   "ERROR") \
    x(Fatal,   "FATAL") \

ENUM_STRUCT_WITH_NAMES(Volume, VOLUME_ITEMS);

    struct ChannelDecl
    {
        char const* name;
        Volume minVolume = (Volume)0;
    };

    struct Channel
    {
        Volume minVolume = (Volume)0;
    };

    struct Entry
    {
        char const*         msg;
        char const*         channelName;
        char const*         sourceFile;
        f32                 timeSeconds;
        i32                 sourceLine;
        i32                 msgLen;         // Not counting terminator
        u32                 threadId;
        Volume              volume;
    };

#define LOG_ENDPOINT(x) void x( Logging::Entry const& entry, void* userdata )
    typedef LOG_ENDPOINT(EndpointFunc);

    struct EndpointInfo
    {
        char const*         name;
        EndpointFunc*       func;
        void*               userdata;
        u32                 id;
    };

    struct State
    {
        Hashtable<char const*, Channel>     channels;
        Array<EndpointInfo>                 endpoints;
        SyncRingBuffer<char>                msgBuffer;
        // TODO Assert when full
        SyncRingBuffer<Entry>               entryQueue;
        Semaphore                           entrySemaphore;
        MemoryArena                         threadArena;
        MemoryArena                         threadTmpArena;
        std::atomic<Platform::ThreadHandle> thread;
    };


    void Init( State* state, Buffer<ChannelDecl> channels );
    void Shutdown( State* state );

    void AttachEndpoint( StaticStringHash name, EndpointFunc* endpoint, void* userdata = nullptr );

    // NOTE These macros should seamlesly work when passing a va_list directly instead of var args
#define LogD( channel, msg, ... )    Logging::LogInternal( channel, Logging::Volume::Debug,     __FILE__, __LINE__, msg, ##__VA_ARGS__ )
#define LogI( channel, msg, ... )    Logging::LogInternal( channel, Logging::Volume::Info,      __FILE__, __LINE__, msg, ##__VA_ARGS__ )
#define LogW( channel, msg, ... )    Logging::LogInternal( channel, Logging::Volume::Warning,   __FILE__, __LINE__, msg, ##__VA_ARGS__ )
#define LogE( channel, msg, ... )    Logging::LogInternal( channel, Logging::Volume::Error,     __FILE__, __LINE__, msg, ##__VA_ARGS__ )
#define LogF( channel, msg, ... )    Logging::LogInternal( channel, Logging::Volume::Fatal,     __FILE__, __LINE__, msg, ##__VA_ARGS__ )
    void LogInternal( char const* channelName, Volume volume, char const* file, int line, char const* msg, ... );
    void LogInternalVA( char const* channelName, Volume volume, char const* file, int line, char const* msg, va_list args );
} // namespace Logging

