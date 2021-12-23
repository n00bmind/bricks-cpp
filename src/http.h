#pragma once

namespace Http
{
    enum Flags
    {
        None = 0,
        VerifyHostCert,
        UseExternalCertFile,
    };

    struct Header
    {
        char const* name;
        String value;
    };

    struct Response
    {
        String url;
        Array<u8> rawData;
        String headers;
        Buffer<u8> body;
        char const* reason;
        i32 statusCode;
        // TODO If this is not found in the response data.. what should the default be?
        bool close = true;
        // TODO Turn this into an enum/string probably?
        bool errored;
    };

    typedef void(*Callback)( const Response& response, void* userdata );

#define METHOD(x) \
    x(Get,  "GET") \
    x(Post, "POST") \

    ENUM_STRUCT_WITH_NAMES(Method, METHOD)

    struct Request
    {
        struct
        {
            mbedtls_net_context         fd;
            mbedtls_ssl_context         context;
            mbedtls_ssl_config          config;
            mbedtls_entropy_context     entropy;
            mbedtls_ctr_drbg_context    ctrDrbg;
            mbedtls_x509_crt            cacert;
        } tls;

        Array<Header> headers;
        Callback callback;
        String bodyData;
        void* userData;

        // FIXME Fix String so it actually copies in its copy constructor!
        String host;
        String port;
        String resource;
        Method method;
        bool https;
        bool ready;
    };

    struct State
    {
        SyncQueue<Request> requestQueue;
        Semaphore requestSemaphore;
        MemoryArena threadArena;
        MemoryArena threadTmpArena;
        Platform::ThreadHandle thread;
        atomic_bool threadRunning;
        bool initialized;
    };


    bool Init( State* state );
    void Shutdown( State* state );

    bool Get( State* state, char const* url, Array<Header> const& headers, Callback callback, void* userData = nullptr, u32 flags = 0 );
    bool Get( State* state, char const* url, Callback callback, void* userData = nullptr, u32 flags = 0 );

} // namespace Http
