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


    bool Init();
    void Shutdown();

    bool Get( char const* url, Callback callback, void* userData = nullptr, u32 flags = 0 );

} // namespace Http
