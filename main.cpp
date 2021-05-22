// Test servers:
// https://stackoverflow.com/questions/5725430/http-test-server-accepting-get-post-requests

#include <stdio.h>

#define CPPHTTPLIB_OPENSSL_SUPPORT
#include "httplib.h"


char const* errMsgs[] =
{
  "Success",
  "Unknown",
  "Connection",
  "BindIPAddress",
  "Read",
  "Write",
  "ExceedRedirectCount",
  "Canceled",
  "SSLConnection",
  "SSLLoadingCerts",
  "SSLServerVerification",
  "UnsupportedMultipartBoundaryChars",
  "Compression",
};

int main()
{
    // HTTP
    httplib::Client cli("https://httpbin.org");

    // HTTPS
    //httplib::Client cli("https://cpp-httplib-server.yhirose.repl.co");
    //auto res = cli.Get("/hi");

    auto res = cli.Get("/ip");
    if( res )
    {
        printf( "Status: %d\n", res->status );
        printf( "Body: %s\n", res->body.c_str() );
    }
    else
    {
        printf( "ERROR: %s\n", errMsgs[(int)res.error()] );
    }

    return 0;
}
