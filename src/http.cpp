#include "ca_cert.h"

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
        String url;
    };

    static bool Connect( char const* url, u32 flags = 0 )
    {
        if( !s_initialized )
        {
            if( !Init() )
                return false;
        }

        Request request = {};
        request.url = String::Clone( url );

        // Make another copy where we can write on
        String urlString = String::Clone( url );
        url = urlString.CStr();


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
        char* hostEnd = (char*)port;

        char* portEnd = nullptr;
        if( port )
        {
            port++;
            portEnd = (char*)StringFind( url, '/' );
        }
        else
        {
            port = https ? "443" : "80";
            hostEnd = (char*)StringFind( url, '/' );
        }

        if( portEnd )
            *portEnd = 0;
        if( hostEnd )
            *hostEnd = 0;


        // Init mbedtls stuff
        int ret;
        if( https )
        {
            mbedtls_net_init( &request.tls.fd );
            mbedtls_ssl_init( &request.tls.context );
            mbedtls_ssl_config_init( &request.tls.config );
            mbedtls_ctr_drbg_init( &request.tls.ctrDrbg );
            mbedtls_entropy_init( &request.tls.entropy );
        }

        // TODO Investigate how to do proper non-blocking connects
        // The old Https code had its own mbedtls_net_connect_timeout, which seems like it heavily borrows from the
        // mbedtls_net_connect source code. The problem is afaik connect & select will still block.. so?
        if( (ret = mbedtls_net_connect( &request.tls.fd, host, port, MBEDTLS_NET_PROTO_TCP )) != 0 )
        {
            //LogError( Net, "mbedtls_net_connect returned %d", ret );
            return false;
        }

        // Make socket ops non blocking
        // TODO Review what the implications are
        if( (ret = mbedtls_net_set_nonblock( &request.tls.fd )) != 0 )
        {
            //LogError( Net, "mbedtls_net_set_nonblock returned %d", ret );
            return false;
        }

        if( https )
        {
            if( (ret = mbedtls_ssl_config_defaults( &request.tls.config,
                                                    MBEDTLS_SSL_IS_CLIENT, MBEDTLS_SSL_TRANSPORT_STREAM, MBEDTLS_SSL_PRESET_DEFAULT ))
                != 0 )
            {
                //LogError( Net, "mbedtls_ssl_config_defaults returned %d\n\n", ret );
                return false;
            }

            // TODO Check if any of this can be done in Init (assuming we're gonna want to verify at some point later)
            if( flags & VerifyHostCert )
            {
                mbedtls_x509_crt_init( &request.tls.cacert );

                if( flags & UseExternalCertFile )
                {
                    constexpr const char *cafile = "/path/to/trusted-ca-list.pem";
                    if( (ret = mbedtls_x509_crt_parse_file( &request.tls.cacert, cafile )) != 0 )
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
                        if( (ret = mbedtls_x509_crt_parse( &request.tls.cacert, (u8*)ca_crt_rsa[i], strlen( ca_crt_rsa[i] ) + 1 )) != 0 )
                        {
                            // TODO Log error
                            return false;
                        }
                    }
                }

                mbedtls_ssl_conf_ca_chain( &request.tls.config, &request.tls.cacert, nullptr );
            }
            else
            {
                mbedtls_ssl_conf_authmode( &request.tls.config, MBEDTLS_SSL_VERIFY_NONE );
            }

            if( (ret = mbedtls_ctr_drbg_seed( &request.tls.ctrDrbg, mbedtls_entropy_func, &request.tls.entropy, nullptr, 0 )) != 0 )
            {
                // TODO 
                //LogError( Net, "mbedtls_ctr_drbg_seed returned %d", ret );
                return false;
            }
            mbedtls_ssl_conf_rng( &request.tls.config, mbedtls_ctr_drbg_random, &request.tls.ctrDrbg ); 
#if CONFIG_DEBUG
            mbedtls_ssl_conf_dbg( &request.tls.config, nullptr, stdout );
#endif

            if( (ret = mbedtls_ssl_setup( &request.tls.context, &request.tls.config )) != 0 )
            {
                //LogError( Net, "mbedtls_ssl_setup returned %d\n\n", ret );
                return false;
            }

            if( (ret = mbedtls_ssl_set_hostname( &request.tls.context, host )) != 0 )
            {
                //LogError( Net, "mbedtls_ssl_set_hostname returned %d\n\n", ret );
                return false;
            }
            mbedtls_ssl_set_bio( &request.tls.context, &request.tls.fd, mbedtls_net_send, mbedtls_net_recv, nullptr );

            // Do the dance
            while( (ret = mbedtls_ssl_handshake( &request.tls.context )) != 0 )
            {
                if( ret != MBEDTLS_ERR_SSL_WANT_READ && ret != MBEDTLS_ERR_SSL_WANT_WRITE )
                {
                    //LogError( Net, "mbedtls_ssl_handshake returned -0x%x\n\n", (unsigned int)-ret );
                    return false;
                }
            }

            if( (flags & VerifyHostCert) && (mbedtls_ssl_get_verify_result( &request.tls.context ) != 0) )
            {
                //return MBEDTLS_ERR_X509_CERT_VERIFY_FAILED;
                return false;
            }
        }

        return true;
    }

    // TODO Headers
    // TODO We're gonna want some way of reusing connections to the same server etc
    bool Get( char const* url, Callback callback, void* userData /*= nullptr*/, u32 flags /*= 0*/ )
    {
        if( !Connect( url ) )
            return false;

        return true;
    }


} // namespace Http
