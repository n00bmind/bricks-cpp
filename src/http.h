#pragma once

namespace Http
{
    enum Flags
    {
        None = 0,
        VerifyHostCert,
        UseExternalCertFile,
    };

    struct Response
    {
        String url;
        String rawContent;
        int statusCode;
    };

    typedef void(*Callback)( const Response& response, void* userdata );


    bool Init();
    void Shutdown();

    bool Get( char const* url, Callback callback, void* userData = nullptr, u32 flags = 0 );

} // namespace Http
