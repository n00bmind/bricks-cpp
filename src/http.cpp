#include "ca_cert.h"

#define HTTP_PRINT_CONTENTS 1

// SPEC: https://datatracker.ietf.org/doc/html/rfc2616

namespace Http
{
    static bool s_initialized = false;


    bool Init()
    {
        if( s_initialized )
            return true;

        // TODO Move to the platform layer
#ifdef _WIN32
        WSADATA wsaData;
        int ret = WSAStartup(MAKEWORD(2,2), &wsaData);
        ASSERT( ret == 0 );
#endif

        s_initialized = true;
        return true;
    }

    void Shutdown()
    {
        if( !s_initialized )
            return;

        // TODO Move to the platform layer
#ifdef _WIN32
        int ret = WSACleanup();
        ASSERT( ret == 0 );
#endif

        s_initialized = false;
    }

    
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

        // FIXME Fix String so it actually copies in its copy constructor!
        String host;
        String port;
        String resource;
        Method method;
        bool https;
    };

    static void Close( Request* request )
    {
        if( request->https )
        {
            mbedtls_ssl_close_notify( &request->tls.context );

            mbedtls_ssl_free( &request->tls.context );
            mbedtls_ssl_config_free( &request->tls.config );
            mbedtls_ctr_drbg_free( &request->tls.ctrDrbg );
            mbedtls_entropy_free( &request->tls.entropy );
            mbedtls_x509_crt_free( &request->tls.cacert );
        }

        mbedtls_net_free( &request->tls.fd );
    }

    static bool Connect( char const* url, Request* request, u32 flags = 0 )
    {
        if( !s_initialized )
        {
            if( !Init() )
                return false;
        }

        *request = {};


        // Parse URL
        bool https = false;
#define HTTP "http://"
#define HTTPS "https://"
        if( StringStartsWith( url, HTTPS ) )
        {
            url = url + sizeof( HTTPS ) - 1;
            https = true;
        }
        else if( StringStartsWith( url, HTTP ) )
        {
            url = url + sizeof(HTTP) - 1;
        }
#undef HTTP
#undef HTTPS
        char const* host = url;

        char const* port = StringFind( url, ':' );
        char const* hostEnd = port;

        char const* portEnd = nullptr;
        if( port )
        {
            port++;
            portEnd = StringFind( url, '/' );
        }
        else
        {
            port = https ? "443" : "80";
            hostEnd = StringFind( url, '/' );
        }

        request->port = String::Clone( port, portEnd );
        request->host = String::Clone( host, hostEnd );
        if( hostEnd )
            request->resource = String::Clone( hostEnd );
        else
            request->resource = "/";

        request->https = https;


        // Init mbedtls stuff
        int ret = 0;
        if( https )
        {
            mbedtls_net_init( &request->tls.fd );
            mbedtls_ssl_init( &request->tls.context );
            mbedtls_ssl_config_init( &request->tls.config );
            mbedtls_ctr_drbg_init( &request->tls.ctrDrbg );
            mbedtls_entropy_init( &request->tls.entropy );
        }

        DEFER
        (
            if( ret != 0 ) 
                Close( request );
        )

        // TODO Investigate how to do proper non-blocking connects
        // The old Https code had its own mbedtls_net_connect_timeout, which seems like it heavily borrows from the
        // mbedtls_net_connect source code. The problem is afaik connect & select will still block.. so?
        if( (ret = mbedtls_net_connect( &request->tls.fd, request->host.CStr(), request->port.CStr(), MBEDTLS_NET_PROTO_TCP )) != 0 )
        {
            //LogError( Net, "mbedtls_net_connect returned %d", ret );
            return false;
        }

        // Make socket ops non blocking
        // TODO Review what the implications are
        if( (ret = mbedtls_net_set_nonblock( &request->tls.fd )) != 0 )
        {
            //LogError( Net, "mbedtls_net_set_nonblock returned %d", ret );
            return false;
        }

        if( https )
        {
            if( (ret = mbedtls_ssl_config_defaults( &request->tls.config,
                                                    MBEDTLS_SSL_IS_CLIENT, MBEDTLS_SSL_TRANSPORT_STREAM, MBEDTLS_SSL_PRESET_DEFAULT ))
                != 0 )
            {
                //LogError( Net, "mbedtls_ssl_config_defaults returned %d\n\n", ret );
                return false;
            }

            // TODO Check if any of this can be done in Init (assuming we're gonna want to verify at some point later)
            if( flags & VerifyHostCert )
            {
                mbedtls_x509_crt_init( &request->tls.cacert );

                if( flags & UseExternalCertFile )
                {
                    constexpr const char *cafile = "/path/to/trusted-ca-list.pem";
                    if( (ret = mbedtls_x509_crt_parse_file( &request->tls.cacert, cafile )) != 0 )
                    {
                        //LogError(  Net, "mbedtls_x509_crt_parse returned -0x%x\n\n", -ret );
                        return false;
                    }
                }
                else
                {
                    for( int i = 0; i < ARRAYCOUNT(ca_crt_rsa); ++i )
                    {
                        // +1 to account for the null terminator
                        if( (ret = mbedtls_x509_crt_parse( &request->tls.cacert, (u8*)ca_crt_rsa[i], strlen( ca_crt_rsa[i] ) + 1 )) != 0 )
                        {
                            // TODO Log error
                            return false;
                        }
                    }
                }

                mbedtls_ssl_conf_ca_chain( &request->tls.config, &request->tls.cacert, nullptr );
            }
            else
            {
                mbedtls_ssl_conf_authmode( &request->tls.config, MBEDTLS_SSL_VERIFY_NONE );
            }

            if( (ret = mbedtls_ctr_drbg_seed( &request->tls.ctrDrbg, mbedtls_entropy_func, &request->tls.entropy, nullptr, 0 )) != 0 )
            {
                // TODO 
                //LogError( Net, "mbedtls_ctr_drbg_seed returned %d", ret );
                return false;
            }
            mbedtls_ssl_conf_rng( &request->tls.config, mbedtls_ctr_drbg_random, &request->tls.ctrDrbg ); 
#if CONFIG_DEBUG
            mbedtls_ssl_conf_dbg( &request->tls.config, nullptr, stdout );
#endif

            if( (ret = mbedtls_ssl_setup( &request->tls.context, &request->tls.config )) != 0 )
            {
                //LogError( Net, "mbedtls_ssl_setup returned %d\n\n", ret );
                return false;
            }

            if( (ret = mbedtls_ssl_set_hostname( &request->tls.context, request->host.CStr() )) != 0 )
            {
                //LogError( Net, "mbedtls_ssl_set_hostname returned %d\n\n", ret );
                return false;
            }
            mbedtls_ssl_set_bio( &request->tls.context, &request->tls.fd, mbedtls_net_send, mbedtls_net_recv, nullptr );

            // Do the dance
            while( (ret = mbedtls_ssl_handshake( &request->tls.context )) != 0 )
            {
                if( ret != MBEDTLS_ERR_SSL_WANT_READ && ret != MBEDTLS_ERR_SSL_WANT_WRITE )
                {
                    //LogError( Net, "mbedtls_ssl_handshake returned -0x%x\n\n", (unsigned int)-ret );
                    return false;
                }
            }

            if( (flags & VerifyHostCert) && (mbedtls_ssl_get_verify_result( &request->tls.context ) != 0) )
            {
                //return MBEDTLS_ERR_X509_CERT_VERIFY_FAILED;
                return false;
            }
        }

        return true;
    }

    static String BuildRequest( Request const& request, Array<Header> const& headers, String bodyData )
    {
        // Dump lowercased user headers into a hashtable
        static constexpr int defaultHeadersCount = 6;
        Hashtable< String, String > tmpHeaders( headers.count + defaultHeadersCount, CTX_TMPALLOC );

        for( Header const& h : headers )
            tmpHeaders.Put( String::Clone( h.name ).ToLowercase(), h.value );

        // Add common and 'mandatory' headers to the user provided set
        tmpHeaders.Put( "user-agent"_str, "BricksEngine/1.0"_str );
        tmpHeaders.Put( "host"_str, String::FromFormatTmp( "%s:%s", request.host.CStr(), request.port.CStr() ) );
        if( bodyData )
            tmpHeaders.Put( "content-length"_str, String::FromFormatTmp( "%d", bodyData.length ) ); 
        tmpHeaders.PutIfNotFound( "connection"_str, "Keep-Alive"_str );
        tmpHeaders.PutIfNotFound( "accept"_str, "*/*"_str );
        tmpHeaders.PutIfNotFound( "content-type"_str, "application/json; charset=utf-8"_str );


        // TODO Keep any interesting info in the request
#if 0
        Data* newReq = (Data*)&request;
        // Retrieve request data from tmpHeaders
        if( String* h = tmpHeaders.Get( "Content-Type"_str ) )
            newReq->contentType = String::Clone( *h );
        if( String* h = tmpHeaders.Get( "Content-Length"_str ) )
        {
            bool ret = h->ToI32( &newReq->contentLength );
            ASSERT( ret );
        }
        if( String* h = tmpHeaders.Get( "Cookie"_str ) )
            newReq->cookie = String::Clone( *h );
        if( String* h = tmpHeaders.Get( "Referer"_str ) )
            newReq->referer = String::Clone( *h );
        if( String* h = tmpHeaders.Get( "Transfer-Encoding"_str ) )
        {
            if( *h == "chunked"_str )
                newReq->chunked = true;
        }
        if( String* h = tmpHeaders.Get( "Connection"_str ) )
        {
            if( *h == "close"_str )
                newReq->close = true;
        }
#endif
        // TODO Do we want to copy this?
#if 0
        newReq->headers = tmpHeaders;
#endif

        char const* method = request.method.Name();

        StringBuilder sb;
        sb.AppendFormat( "%s %s HTTP/1.1\r\n", method, request.resource.CStr() );
        for( auto it = tmpHeaders.Items(); it; ++it )
            sb.AppendFormat( "%s: %s\r\n", (*it).key.CStr(), (*it).value.CStr() );
        // TODO Should we skip this when there's no body?
        sb.Append( "\r\n"_str );
        if( bodyData )
            sb.AppendFormat( "%s", bodyData.CStr() );

        String result = sb.ToStringTmp();
#if HTTP_PRINT_CONTENTS
        printf("--- REQ:\n%s---\n", result.CStr() );
#endif

        return result;
    }

    static bool Write( Request* request, Buffer<u8> buffer )
    {
        int ret = 0, error = 0, slen = 0;

        while( true )
        {
            if( request->https )
                ret = mbedtls_ssl_write( &request->tls.context, (u_char *)&buffer.data[slen], (size_t)(buffer.length - slen) );
            else
                ret = mbedtls_net_send( &request->tls.fd, (u_char *)&buffer.data[slen], (size_t)(buffer.length - slen) );

            if( ret == MBEDTLS_ERR_SSL_WANT_WRITE )
                continue;
            else if( ret <= 0 )
            {
                error = ret;
                break;
            }

            slen += ret;
            if( slen >= buffer.length )
                break;
        }

        if( error )
        {
            char errorBuf[128];
            mbedtls_strerror( error, errorBuf, sizeof(errorBuf) );
            LOGE( /*Net,*/ "Write error (%d): %s", error, errorBuf );

            Close( request );
        }

        return error == 0;
    }

    // TODO Chunked requests
#if 0
    static int Https::WriteChunked( char *data, int len )
    {
        char        str[10];
        int         ret, l;


        if(NULL == this || len <= 0) return -1;

        if(this->request.chunked == TRUE)
        {
            l = snprintf(str, 10, "%x\r\n", len);

            if ((ret = https_write(this, str, l)) != l)
            {
                https_close(this);
                _error = ret;

                return -1;
            }
        }

        if((ret = https_write(this, data, len)) != len)
        {
            https_close(this);
            _error = ret;

            return -1;
        }

        if(this->request.chunked == TRUE)
        {
            if ((ret = https_write(this, "\r\n", 2)) != 2)
            {
                https_close(this);
                _error = ret;

                return -1;
            }
        }

        return len;
    }

    static int Https::WriteEnd()
    {
        char        str[10];
        int         ret, len;

        if (NULL == this) return -1;

        if(this->request.chunked == TRUE)
        {
            len = snprintf(str, 10, "0\r\n\r\n");
        }
        else
        {
            len = snprintf(str, 10, "\r\n\r\n");
        }

        if ((ret = https_write(this, str, len)) != len)
        {
            https_close(this);
            _error = ret;

            return -1;
        }

        return len;
    }
#endif

    static bool ParseResponse( Response* response )
    {
        // Don't count terminator
        String responseString( (char const*)response->rawData.data, response->rawData.count - 1 );
#if HTTP_PRINT_CONTENTS
        printf("--- RSP:\n%s---\n", responseString.CStr() );
#endif

        while( String line = responseString.ConsumeLine() )
        {
            if( line.StartsWith( "\r\n" ) )
            {
                // Header is over. Save the remainder as the body
                // TODO Terminator?
                response->body = { (u8*)responseString.data, responseString.length };
                break;
            }
            else
            {
                if( !response->statusCode )
                {
                    // Parse status line
                    String word = line.ConsumeWord();
                    if( !word || !word.StartsWith( "HTTP/" ) )
                    {
                        LOGE( /*Net,*/ "Bad protocol" );
                        return false;
                    }

                    if( !(word = line.ConsumeWord()) )
                    {
                        LOGE( /*Net,*/ "Bad protocol" );
                        return false;
                    }
                    word.ToI32( &response->statusCode );

                    if( !(word = line.ConsumeWord()) )
                    {
                        LOGE( /*Net,*/ "Bad protocol" );
                        return false;
                    }
                    response->reason = word.data;
                }
                else
                {
                    // Save a ref so we can re-parse them if the user wants the headers at some point
                    if( !response->headers )
                        response->headers = String::Ref( responseString );

                    // Parse headers
                    // TODO Fill up relevant info in the Response

                    String word = line.ConsumeWord();
                    if( !word || !word.EndsWith( ":" ) )
                    {
                        LOGW( /*Net,*/ "Malformed response header: '%s'", line.CStr() );
                        continue;
                    }
                    char const* name = word.data;

                    if( !(word = line.ConsumeWord()) )
                    {
                        LOGW( /*Net,*/ "Malformed response header: '%s'", line.CStr() );
                        continue;
                    }

                    Header h = { name, std::move( word ) };
                }
            }
        }

        response->done = true;
        return true;
    }

    // Return true if there's more data to read
    bool Read( Request* request, BucketArray<Array<u8>>* readChunks, Response* response )
    {
        ASSERT( !response->done );

        static constexpr int chunkSize = 4096;
        // Get a new chunk or reuse the last one
        Array<u8>* chunk = nullptr;
        if( readChunks->Empty() || (*readChunks->Last()).Available() == 0 )
        {
            chunk = readChunks->PushEmpty( false );
            INIT( *chunk )( chunkSize, CTX_TMPALLOC );
        }
        else
            chunk = &(*readChunks->Last());

        // Read into the new chunk
        int ret = 0, error = 0;
        if( request->https )
            ret = mbedtls_ssl_read( &request->tls.context, (u_char *)chunk->end(), (size_t)chunk->Available() );
        else
            ret = mbedtls_net_recv_timeout( &request->tls.fd, (u_char *)chunk->end(), (size_t)chunk->Available(), 5000 );

        if( ret == MBEDTLS_ERR_SSL_WANT_READ )
            return true;
        else if( ret <= 0 )
        {
            if( ret == MBEDTLS_ERR_SSL_PEER_CLOSE_NOTIFY )
            {
                // Connection closed gracefully
                Close( request );
            }
            else
            {
                error = ret;

                if( ret == 0 || ret == MBEDTLS_ERR_NET_CONN_RESET )
                    LOGE( /*Net,*/ "Connection closed by peer" );
                else
                {
                    char errorBuf[128];
                    mbedtls_strerror( error, errorBuf, sizeof(errorBuf) );
                    LOGE( /*Net,*/ "Read error (%d): %s", error, errorBuf );
                }

                Close( request );
                return false;
            }
        }

        if( ret > 0 )
            chunk->count += ret;

        // TODO Technically, we should parse the http contents to ensure we've finished receiving all data
        // (see https://github.com/ARMmbed/mbedtls/blob/146247de712f00220226829dcf13e99f62133ad6/programs/ssl/ssl_client2.c#L2627)
        // For now, just try again if we last read a full chunk
        bool completed = ret < chunkSize;
        if( completed )
        {
            // Compact down and null terminate the buffer
            ASSERT( response->rawData.capacity == 0 );

            int totalSize = 0;
            for( Array<u8> const& c : *readChunks )
                totalSize += c.count;
            INIT( response->rawData )( totalSize + 1 );

            for( Array<u8> const& c : *readChunks )
                response->rawData.Append( c );
            // TODO For binary data we probably want to avoid this?
            response->rawData.Push( 0 );

            if( ParseResponse( response ) )
            {
                if( response->close )
                    Close( request );

                return false;
            }

            ASSERT( false, "Bad response" );
            return false;
        }
        else
            return true;
    }

    int ReadBlocking( Request* request )
    {
        Response response = {};
        BucketArray<Array<u8>> readChunks( 8, CTX_TMPALLOC );

        while(1)
        {
            if( !Read( request, &readChunks, &response ) )
                break;
        }

        return response.statusCode;
    }

    // TODO We're gonna want some way of reusing connections to the same server etc
    bool Get( char const* url, Array<Header> const& headers, Callback callback, void* userData /*= nullptr*/, u32 flags /*= 0*/ )
    {
        // TODO Enqueue to the http thread and return immediately

        Request request;
        if( !Connect( url, &request ) )
            return false;

        request.method = Method::Get;
        String reqStr = BuildRequest( request, headers, {} );

        // TODO This is supposed to be non-blocking now? How is that supposed to work?
        if( !Write( &request, reqStr.ToBufferU8() ) )
            return false;

        // TODO Do this non-blocking too
        if( !ReadBlocking( &request ) )
            return false;

        return true;
    }

    bool Get( char const* url, Callback callback, void* userData /*= nullptr*/, u32 flags /*= 0*/ )
    {
        Array<Header> headers;
        return Get( url, headers, callback, userData, flags );
    }

} // namespace Http
