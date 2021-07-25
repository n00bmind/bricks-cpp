// Taken from https://github.com/HISONA/https_client
// Modified to add Windows support
/*
                                 Apache License
                           Version 2.0, January 2004
                        http://www.apache.org/licenses/

   TERMS AND CONDITIONS FOR USE, REPRODUCTION, AND DISTRIBUTION

   1. Definitions.

      "License" shall mean the terms and conditions for use, reproduction,
      and distribution as defined by Sections 1 through 9 of this document.

      "Licensor" shall mean the copyright owner or entity authorized by
      the copyright owner that is granting the License.

      "Legal Entity" shall mean the union of the acting entity and all
      other entities that control, are controlled by, or are under common
      control with that entity. For the purposes of this definition,
      "control" means (i) the power, direct or indirect, to cause the
      direction or management of such entity, whether by contract or
      otherwise, or (ii) ownership of fifty percent (50%) or more of the
      outstanding shares, or (iii) beneficial ownership of such entity.

      "You" (or "Your") shall mean an individual or Legal Entity
      exercising permissions granted by this License.

      "Source" form shall mean the preferred form for making modifications,
      including but not limited to software source code, documentation
      source, and configuration files.

      "Object" form shall mean any form resulting from mechanical
      transformation or translation of a Source form, including but
      not limited to compiled object code, generated documentation,
      and conversions to other media types.

      "Work" shall mean the work of authorship, whether in Source or
      Object form, made available under the License, as indicated by a
      copyright notice that is included in or attached to the work
      (an example is provided in the Appendix below).

      "Derivative Works" shall mean any work, whether in Source or Object
      form, that is based on (or derived from) the Work and for which the
      editorial revisions, annotations, elaborations, or other modifications
      represent, as a whole, an original work of authorship. For the purposes
      of this License, Derivative Works shall not include works that remain
      separable from, or merely link (or bind by name) to the interfaces of,
      the Work and Derivative Works thereof.

      "Contribution" shall mean any work of authorship, including
      the original version of the Work and any modifications or additions
      to that Work or Derivative Works thereof, that is intentionally
      submitted to Licensor for inclusion in the Work by the copyright owner
      or by an individual or Legal Entity authorized to submit on behalf of
      the copyright owner. For the purposes of this definition, "submitted"
      means any form of electronic, verbal, or written communication sent
      to the Licensor or its representatives, including but not limited to
      communication on electronic mailing lists, source code control systems,
      and issue tracking systems that are managed by, or on behalf of, the
      Licensor for the purpose of discussing and improving the Work, but
      excluding communication that is conspicuously marked or otherwise
      designated in writing by the copyright owner as "Not a Contribution."

      "Contributor" shall mean Licensor and any individual or Legal Entity
      on behalf of whom a Contribution has been received by Licensor and
      subsequently incorporated within the Work.

   2. Grant of Copyright License. Subject to the terms and conditions of
      this License, each Contributor hereby grants to You a perpetual,
      worldwide, non-exclusive, no-charge, royalty-free, irrevocable
      copyright license to reproduce, prepare Derivative Works of,
      publicly display, publicly perform, sublicense, and distribute the
      Work and such Derivative Works in Source or Object form.

   3. Grant of Patent License. Subject to the terms and conditions of
      this License, each Contributor hereby grants to You a perpetual,
      worldwide, non-exclusive, no-charge, royalty-free, irrevocable
      (except as stated in this section) patent license to make, have made,
      use, offer to sell, sell, import, and otherwise transfer the Work,
      where such license applies only to those patent claims licensable
      by such Contributor that are necessarily infringed by their
      Contribution(s) alone or by combination of their Contribution(s)
      with the Work to which such Contribution(s) was submitted. If You
      institute patent litigation against any entity (including a
      cross-claim or counterclaim in a lawsuit) alleging that the Work
      or a Contribution incorporated within the Work constitutes direct
      or contributory patent infringement, then any patent licenses
      granted to You under this License for that Work shall terminate
      as of the date such litigation is filed.

   4. Redistribution. You may reproduce and distribute copies of the
      Work or Derivative Works thereof in any medium, with or without
      modifications, and in Source or Object form, provided that You
      meet the following conditions:

      (a) You must give any other recipients of the Work or
          Derivative Works a copy of this License; and

      (b) You must cause any modified files to carry prominent notices
          stating that You changed the files; and

      (c) You must retain, in the Source form of any Derivative Works
          that You distribute, all copyright, patent, trademark, and
          attribution notices from the Source form of the Work,
          excluding those notices that do not pertain to any part of
          the Derivative Works; and

      (d) If the Work includes a "NOTICE" text file as part of its
          distribution, then any Derivative Works that You distribute must
          include a readable copy of the attribution notices contained
          within such NOTICE file, excluding those notices that do not
          pertain to any part of the Derivative Works, in at least one
          of the following places: within a NOTICE text file distributed
          as part of the Derivative Works; within the Source form or
          documentation, if provided along with the Derivative Works; or,
          within a display generated by the Derivative Works, if and
          wherever such third-party notices normally appear. The contents
          of the NOTICE file are for informational purposes only and
          do not modify the License. You may add Your own attribution
          notices within Derivative Works that You distribute, alongside
          or as an addendum to the NOTICE text from the Work, provided
          that such additional attribution notices cannot be construed
          as modifying the License.

      You may add Your own copyright statement to Your modifications and
      may provide additional or different license terms and conditions
      for use, reproduction, or distribution of Your modifications, or
      for any such Derivative Works as a whole, provided Your use,
      reproduction, and distribution of the Work otherwise complies with
      the conditions stated in this License.

   5. Submission of Contributions. Unless You explicitly state otherwise,
      any Contribution intentionally submitted for inclusion in the Work
      by You to the Licensor shall be under the terms and conditions of
      this License, without any additional terms or conditions.
      Notwithstanding the above, nothing herein shall supersede or modify
      the terms of any separate license agreement you may have executed
      with Licensor regarding such Contributions.

   6. Trademarks. This License does not grant permission to use the trade
      names, trademarks, service marks, or product names of the Licensor,
      except as required for reasonable and customary use in describing the
      origin of the Work and reproducing the content of the NOTICE file.

   7. Disclaimer of Warranty. Unless required by applicable law or
      agreed to in writing, Licensor provides the Work (and each
      Contributor provides its Contributions) on an "AS IS" BASIS,
      WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or
      implied, including, without limitation, any warranties or conditions
      of TITLE, NON-INFRINGEMENT, MERCHANTABILITY, or FITNESS FOR A
      PARTICULAR PURPOSE. You are solely responsible for determining the
      appropriateness of using or redistributing the Work and assume any
      risks associated with Your exercise of permissions under this License.

   8. Limitation of Liability. In no event and under no legal theory,
      whether in tort (including negligence), contract, or otherwise,
      unless required by applicable law (such as deliberate and grossly
      negligent acts) or agreed to in writing, shall any Contributor be
      liable to You for damages, including any direct, indirect, special,
      incidental, or consequential damages of any character arising as a
      result of this License or out of the use or inability to use the
      Work (including but not limited to damages for loss of goodwill,
      work stoppage, computer failure or malfunction, or any and all
      other commercial damages or losses), even if such Contributor
      has been advised of the possibility of such damages.

   9. Accepting Warranty or Additional Liability. While redistributing
      the Work or Derivative Works thereof, You may choose to offer,
      and charge a fee for, acceptance of support, warranty, indemnity,
      or other liability obligations and/or rights consistent with this
      License. However, in accepting such obligations, You may act only
      on Your own behalf and on Your sole responsibility, not on behalf
      of any other Contributor, and only if You agree to indemnify,
      defend, and hold each Contributor harmless for any liability
      incurred by, or claims asserted against, such Contributor by reason
      of your accepting any such warranty or additional liability.

   END OF TERMS AND CONDITIONS

   APPENDIX: How to apply the Apache License to your work.

      To apply the Apache License to your work, attach the following
      boilerplate notice, with the fields enclosed by brackets "{}"
      replaced with your own identifying information. (Don't include
      the brackets!)  The text should be enclosed in the appropriate
      comment syntax for the file format. We also recommend that a
      file or class name and description of purpose be included on the
      same "printed page" as the copyright notice for easier
      identification within third-party archives.

   Copyright {yyyy} {name of copyright owner}

   Licensed under the Apache License, Version 2.0 (the "License");
   you may not use this file except in compliance with the License.
   You may obtain a copy of the License at

       http://www.apache.org/licenses/LICENSE-2.0

   Unless required by applicable law or agreed to in writing, software
   distributed under the License is distributed on an "AS IS" BASIS,
   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
   See the License for the specific language governing permissions and
   limitations under the License.
*/
//
// Created by HISONA on 2016. 2. 29..
//

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <signal.h>

#ifdef _WIN32

#define _CRT_NONSTDC_NO_DEPRECATE
#include <winsock2.h>
#include <ws2tcpip.h>
#define strncasecmp _strnicmp

using socket_t = SOCKET;

#else

#include <sys/socket.h>
#include <unistd.h>
#include <netdb.h>

using socket_t = int;
#define INVALID_SOCKET (-1)

#endif // #ifdef _WIN32


#include "ca_cert.h"
#include "https.h"

/* TODO Fix these! */
#ifdef _MSC_VER
#pragma warning( push )
#pragma warning( disable : 4244 )
#pragma warning( disable : 4267 )
#pragma warning( disable : 4805 )
#endif


// TODO mbedTLS supports allocating all memory from a static buffer: https://tls.mbed.org/kb/how-to/using-static-memory-instead-of-the-heap



/*---------------------------------------------------------------------*/
// TODO Remove
static int _error;

/*---------------------------------------------------------------------*/
char *strtoken(char *src, char *dst, int size);

static int http_header(Http *hi, char *param);

static int https_init(Http *hi, bool https, bool verify);
static int https_close(Http *hi);

/*---------------------------------------------------------------------------*/
char *strtoken(char *src, char *dst, int size)
{
    char *p, *st, *ed;
    int  len = 0;

    // l-trim
    p = src;

    while(TRUE)
    {
        if((*p == '\n') || (*p == 0)) return NULL; /* value is not exists */
        if((*p != ' ') && (*p != '\t')) break;
        p++;
    }

    st = p;
    while(TRUE)
    {
        ed = p;
        if(*p == ' ') {
            p++;
            break;
        }
        if((*p == '\n') || (*p == 0)) break;
        p++;
    }

    // r-trim
    while(TRUE)
    {
        ed--;
        if(st == ed) break;
        if((*ed != ' ') && (*ed != '\t')) break;
    }

    len = (int)(ed - st + 1);
    if((size > 0) && (len >= size)) len = size - 1;

    strncpy(dst, st, len);
    dst[len]=0;

    return p;
}

/*---------------------------------------------------------------------*/
static int parse_url(char const* src_url, int *https, char *host, char *port, char *urlOut )
{
    char* p1;
    char* p2;
    char str[1024] = {};

    String urlString = String::CloneTmp( src_url );
    char* url = (char*)urlString.CStr();

    if(strncmp( url, "http://", 7)==0) {
        p1=&url[7];
        *https = 0;
    } else if(strncmp( url, "https://", 8)==0) {
        p1=&url[8];
        *https = 1;
    } else {
        p1 = &url[0];
        *https = 0;
    }

    if((p2=strstr(p1, "/")) == NULL)
    {
        sprintf(str, "%s", p1);
        sprintf(urlOut , "/");
    }
    else
    {
        strncpy(str, p1, p2-p1);
        snprintf(urlOut , 256, "%s", p2);
    }

    if((p1=strstr(str, ":")) != NULL)
    {
        *p1=0;
        snprintf(host, 256, "%s", str);
        snprintf(port, 5, "%s", p1+1);
    }
    else
    {
        snprintf(host, 256, "%s", str);

        if(*https == 0)
            snprintf(port, 5, "80");
        else
            snprintf(port, 5, "443");
    }

    return 0;

}

/*---------------------------------------------------------------------*/
static int http_header(Http *hi, char *param)
{
    char *token;
    char t1[256], t2[256];
    int  len;

    token = param;

    if((token=strtoken(token, t1, 256)) == 0) return -1;
    if((token=strtoken(token, t2, 256)) == 0) return -1;

    if(strncasecmp(t1, "HTTP", 4) == 0)
    {
        hi->response.status = atoi(t2);
    }
    else if(strncasecmp(t1, "set-cookie:", 11) == 0)
    {
        hi->response.cookie = String::Clone( t2 );
    }
    else if(strncasecmp(t1, "location:", 9) == 0)
    {
        len = (int)strlen(t2);
        strncpy(hi->response.location, t2, len);
        hi->response.location[len] = 0;
    }
    else if(strncasecmp(t1, "content-length:", 15) == 0)
    {
        hi->response.contentLength = atoi(t2);
    }
    else if(strncasecmp(t1, "transfer-encoding:", 18) == 0)
    {
        if(strncasecmp(t2, "chunked", 7) == 0)
        {
            hi->response.chunked = TRUE;
        }
    }
    else if(strncasecmp(t1, "connection:", 11) == 0)
    {
        if(strncasecmp(t2, "close", 5) == 0)
        {
            hi->response.close = TRUE;
        }
    }

    return 1;
}

/*---------------------------------------------------------------------*/
static bool http_parse(Http *hi)
{
    char    *p1, *p2;
    long    len;

    if(hi->r_len <= 0) return false;

    p1 = hi->r_buf;

    while(1)
    {
        if(hi->header_end == FALSE)     // header parser
        {
            if((p2 = strstr(p1, "\r\n")) != NULL)
            {
                len = (long)(p2 - p1);
                *p2 = 0;

                if(len > 0)
                {
                    // printf("header: %s(%ld)\n", p1, len);

                    http_header(hi, p1);
                    p1 = p2 + 2;    // skip CR+LF
                }
                else
                {
                    hi->header_end = TRUE; // reach the header-end.

                    // printf("header_end .... \n");

                    p1 = p2 + 2;    // skip CR+LF

                    if(hi->response.chunked == TRUE)
                    {
                        len = hi->r_len - (p1 - hi->r_buf);
                        if(len > 0)
                        {
                            if((p2 = strstr(p1, "\r\n")) != NULL)
                            {
                                *p2 = 0;

                                if((hi->length = strtol(p1, NULL, 16)) == 0)
                                {
                                    hi->response.chunked = FALSE;
                                }
                                else
                                {
                                    hi->response.contentLength += hi->length;
                                }
                                p1 = p2 + 2;    // skip CR+LF
                            }
                            else
                            {
                                // copy the data as chunked size ...
                                strncpy(hi->r_buf, p1, len);
                                hi->r_buf[len] = 0;
                                hi->r_len = len;
                                hi->length = -1;

                                break;
                            }
                        }
                        else
                        {
                            hi->r_len = 0;
                            hi->length = -1;

                            break;
                        }
                    }
                    else
                    {
                        hi->length = hi->response.contentLength;
                    }
                }
            }
            else
            {
                len = hi->r_len - (p1 - hi->r_buf);
                if(len  > 0)
                {
                    // keep the partial header data ...
                    strncpy(hi->r_buf, p1, len);
                    hi->r_buf[len] = 0;
                    hi->r_len = len;
                }
                else
                    hi->r_len = 0;

                break;
            }
        }
        else    // body parser ...
        {
            if(hi->response.chunked == TRUE && hi->length == -1)
            {
                len = hi->r_len - (p1 - hi->r_buf);
                if(len > 0)
                {
                    if ((p2 = strstr(p1, "\r\n")) != NULL)
                    {
                        *p2 = 0;

                        if((hi->length = strtol(p1, NULL, 16)) == 0)
                            hi->response.chunked = FALSE;
                        else
                            hi->response.contentLength += hi->length;

                        p1 = p2 + 2;    // skip CR+LF
                    }
                    else
                    {
                        // copy the remain data as chunked size ...
                        strncpy(hi->r_buf, p1, len);
                        hi->r_buf[len] = 0;
                        hi->r_len = len;
                        hi->length = -1;

                        break;
                    }
                }
                else
                {
                    hi->r_len = 0;
                    break;
                }
            }
            else
            {
                if(hi->length > 0)
                {
                    len = hi->r_len - (p1 - hi->r_buf);

                    if(len > hi->length)
                    {
                        // copy the data for response ..
                        if(hi->body_len < hi->body_size-1)
                        {
                            if (hi->body_size > (hi->body_len + hi->length))
                            {
                                strncpy(&(hi->body[hi->body_len]), p1, hi->length);
                                hi->body_len += hi->length;
                                hi->body[hi->body_len] = 0;
                            }
                            else
                            {
                                strncpy(&(hi->body[hi->body_len]), p1, hi->body_size - hi->body_len - 1);
                                hi->body_len = hi->body_size - 1;
                                hi->body[hi->body_len] = 0;
                            }
                        }

                        p1 += hi->length;
                        len -= hi->length;

                        if(hi->response.chunked == TRUE && len >= 2)
                        {
                            p1 += 2;    // skip CR+LF
                            hi->length = -1;
                        }
                        else
                        {
                            return false;
                        }
                    }
                    else
                    {
                        // copy the data for response ..
                        if(hi->body_len < hi->body_size-1)
                        {
                            if (hi->body_size > (hi->body_len + len))
                            {
                                strncpy(&(hi->body[hi->body_len]), p1, len);
                                hi->body_len += len;
                                hi->body[hi->body_len] = 0;
                            }
                            else
                            {
                                strncpy(&(hi->body[hi->body_len]), p1, hi->body_size - hi->body_len - 1);
                                hi->body_len = hi->body_size - 1;
                                hi->body[hi->body_len] = 0;
                            }
                        }

                        hi->length -= len;
                        hi->r_len = 0;

                        if(hi->response.chunked == FALSE && hi->length <= 0)
                            return true;

                        break;
                    }
                }
                else
                {
                    if(hi->response.chunked == FALSE)
                        return true;

                    // chunked size check ..
                    if((hi->r_len > 2) && (memcmp(p1, "\r\n", 2) == 0))
                    {
                        p1 += 2;
                        hi->length = -1;
                    }
                    else
                    {
                        hi->length = -1;
                        hi->r_len = 0;
                    }
                }
            }
        }
    }

    return false;
}

/*---------------------------------------------------------------------*/
static int https_init(Http *hi, bool https, bool verify)
{
    memset(hi, 0, sizeof(Http));

    if(https == TRUE)
    {
        mbedtls_ssl_init( &hi->tls.ssl );
        mbedtls_ssl_config_init( &hi->tls.conf );
        mbedtls_x509_crt_init( &hi->tls.cacert );
        mbedtls_ctr_drbg_init( &hi->tls.ctr_drbg );
    }

    mbedtls_net_init(&hi->tls.ssl_fd);

    /* NOTE Moved from https_connect */
    // TODO Should do this statically?
    for( int i = 0; i < (sizeof(ca_crt_rsa) / sizeof(char*)); ++i )
    {
        // +1 to account for the null terminator
        int ret = mbedtls_x509_crt_parse(&hi->tls.cacert, (uint8_t *)ca_crt_rsa[i], strlen(ca_crt_rsa[i]) + 1);
        if( ret != 0 )
        {
            return ret;
        }
    }

    hi->tls.verify = verify;
    hi->url = {};
    hi->url.https = https;

//  printf("https_init ... \n");

    return 0;
}

/*---------------------------------------------------------------------*/
static int https_close(Http *hi)
{
    if(hi->url.https == 1)
    {
        mbedtls_ssl_close_notify(&hi->tls.ssl);
    }

    mbedtls_net_free( &hi->tls.ssl_fd );

    if(hi->url.https == 1)
    {
        mbedtls_x509_crt_free(&hi->tls.cacert);
        mbedtls_ssl_free(&hi->tls.ssl);
        mbedtls_ssl_config_free(&hi->tls.conf);
        mbedtls_ctr_drbg_free(&hi->tls.ctr_drbg);
        mbedtls_entropy_free(&hi->tls.entropy);
    }

//  printf("https_close ... \n");

    return 0;
}

INLINE int close_socket(socket_t sock) {
#ifdef _WIN32
    return closesocket(sock);
#else
    return close(sock);
#endif
}

INLINE bool is_connection_error() {
#ifdef _WIN32
    return WSAGetLastError() != WSAEWOULDBLOCK;
#else
    return errno != EINPROGRESS;
#endif
}

INLINE void PrintError()
{
    // TODO Use the platform for this?
#ifdef _WIN32
    CHAR buffer[2048] = {};
    DWORD error = ::GetLastError();

    DWORD msg_size = FormatMessageA( FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
                                     NULL,
                                     error,
                                     MAKELANGID( LANG_NEUTRAL, SUBLANG_DEFAULT ),
                                     buffer,
                                     ARRAYCOUNT(buffer),
                                     NULL );
    printf( "ERROR [%08x] : %s\n", (u32)error, buffer );
#else
    ASSERT( false, "TO DO" );
#endif
}


/*
 * Initiate a TCP connection with host:port and the given protocol
 * waiting for timeout (ms)
 */
static int mbedtls_net_connect_timeout( mbedtls_net_context *ctx, const char *host, const char *port,
                                        int proto, uint32_t timeout )
{
    int ret;
    struct addrinfo hints, *addr_list, *cur;

    /* TODO Why do we ignore this signal? */
    /* TODO Why don't we restore default behaviour? */
#ifdef SIGPIPE
    signal( SIGPIPE, SIG_IGN );
#endif

    /* Do name resolution with both IPv6 and IPv4 */
    memset( &hints, 0, sizeof( hints ) );
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = proto == MBEDTLS_NET_PROTO_UDP ? SOCK_DGRAM : SOCK_STREAM;
    hints.ai_protocol = proto == MBEDTLS_NET_PROTO_UDP ? IPPROTO_UDP : IPPROTO_TCP;

    ret = getaddrinfo( host, port, &hints, &addr_list );
    if( ret != 0 )
    {
        PrintError();
        return( MBEDTLS_ERR_NET_UNKNOWN_HOST );
    }

    /* Try the sockaddrs until a connection succeeds */
    ret = MBEDTLS_ERR_NET_UNKNOWN_HOST;
    for( cur = addr_list; cur != NULL; cur = cur->ai_next )
    {
        ctx->fd = (int) socket( cur->ai_family, cur->ai_socktype,
                                cur->ai_protocol );
        if( ctx->fd < 0 )
        {
            ret = MBEDTLS_ERR_NET_SOCKET_FAILED;
            continue;
        }

        if( mbedtls_net_set_nonblock( ctx ) < 0 )
        {
            close_socket( ctx->fd );
            ctx->fd = -1;
            ret = MBEDTLS_ERR_NET_CONNECT_FAILED;
            break;
        }

        /* TODO We should be able to use mbedTLS all the way? Check how this compares to 'mbedtls_net_connect' */
        ret = connect( ctx->fd, cur->ai_addr, cur->ai_addrlen );
        if( ret == 0 )
        {
            ret = 0;
            break;
        }
        else if( !is_connection_error() )
        {
            u32            fd = ctx->fd;
            int            opt;
            socklen_t      slen;
            struct timeval tv;
            fd_set         fds;

            while(1)
            {
                FD_ZERO( &fds );
                FD_SET( fd, &fds );

                tv.tv_sec  = timeout / 1000;
                tv.tv_usec = ( timeout % 1000 ) * 1000;

                ret = select( fd+1, NULL, &fds, NULL, timeout == 0 ? NULL : &tv );
                if( ret == -1 )
                {
                    if(errno == EINTR) continue;
                }
                else if( ret == 0 )
                {
                    close_socket( fd );
                    ctx->fd = -1;
                    ret = MBEDTLS_ERR_NET_CONNECT_FAILED;
                }
                else
                {
                    ret = 0;

                    slen = sizeof(int);
                    if( (getsockopt(fd, SOL_SOCKET, SO_ERROR, (char *)&opt, &slen) == 0) && (opt > 0) )
                    {
                        close_socket( fd );
                        ctx->fd = -1;
                        ret = MBEDTLS_ERR_NET_CONNECT_FAILED;
                    }
                }

                break;
            }

            break;
        }
#if 0
        else
        {
            int os_error = WSAGetLastError();
            __debugbreak();
        }
#endif

        close_socket( ctx->fd );
        ctx->fd = -1;
        ret = MBEDTLS_ERR_NET_CONNECT_FAILED;
    }

    freeaddrinfo( addr_list );

    if( (ret == 0) && (mbedtls_net_set_block( ctx ) < 0) )
    {
        close_socket( ctx->fd );
        ctx->fd = -1;
        ret = MBEDTLS_ERR_NET_CONNECT_FAILED;
    }

    return( ret );
}

/*---------------------------------------------------------------------*/
static int https_connect(Http *hi, char *host, char *port)
{
    int ret, https;


    https = hi->url.https;

    if(https == 1)
    {
        mbedtls_entropy_init( &hi->tls.entropy );

        ret = mbedtls_ctr_drbg_seed( &hi->tls.ctr_drbg, mbedtls_entropy_func, &hi->tls.entropy, NULL, 0);
        if( ret != 0 )
        {
            return ret;
        }

        ret = mbedtls_ssl_config_defaults( &hi->tls.conf,
                                           MBEDTLS_SSL_IS_CLIENT,
                                           MBEDTLS_SSL_TRANSPORT_STREAM,
                                           MBEDTLS_SSL_PRESET_DEFAULT );
        if( ret != 0 )
        {
            return ret;
        }

        // TODO Check against the example clients (ssl_client1.c & ssl_client2.c) in the lib's source code
        // because most of this config is done there AFTER the connection is established!?

        /* OPTIONAL is not optimal for security,
         * but makes interop easier in this simplified example */
        mbedtls_ssl_conf_authmode( &hi->tls.conf, MBEDTLS_SSL_VERIFY_OPTIONAL );
        mbedtls_ssl_conf_ca_chain( &hi->tls.conf, &hi->tls.cacert, NULL );
        mbedtls_ssl_conf_rng( &hi->tls.conf, mbedtls_ctr_drbg_random, &hi->tls.ctr_drbg );
        mbedtls_ssl_conf_read_timeout( &hi->tls.conf, 5000 );

        ret = mbedtls_ssl_setup( &hi->tls.ssl, &hi->tls.conf );
        if( ret != 0 )
        {
            return ret;
        }

        ret = mbedtls_ssl_set_hostname( &hi->tls.ssl, host );
        if( ret != 0 )
        {
            return ret;
        }
    }

    ret = mbedtls_net_connect_timeout(&hi->tls.ssl_fd, host, port, MBEDTLS_NET_PROTO_TCP, 5000);
    if( ret != 0 )
    {
        return ret;
    }

    if(https == 1)
    {
        mbedtls_ssl_set_bio(&hi->tls.ssl, &hi->tls.ssl_fd, mbedtls_net_send, mbedtls_net_recv, mbedtls_net_recv_timeout);

        while ((ret = mbedtls_ssl_handshake(&hi->tls.ssl)) != 0)
        {
            if (ret != MBEDTLS_ERR_SSL_WANT_READ && ret != MBEDTLS_ERR_SSL_WANT_WRITE)
            {
                return ret;
            }
        }

        /* In real life, we probably want to bail out when ret != 0 */
        if( hi->tls.verify && (mbedtls_ssl_get_verify_result(&hi->tls.ssl) != 0) )
        {
            return MBEDTLS_ERR_X509_CERT_VERIFY_FAILED;
        }
    }

    return 0;
}

/*---------------------------------------------------------------------*/
static int https_write(Http *hi, char const* buffer, int len)
{
    int ret, slen = 0;

    while(1)
    {
        if(hi->url.https == 1)
            ret = mbedtls_ssl_write(&hi->tls.ssl, (u_char *)&buffer[slen], (size_t)(len-slen));
        else
            ret = mbedtls_net_send(&hi->tls.ssl_fd, (u_char *)&buffer[slen], (size_t)(len-slen));

        if(ret == MBEDTLS_ERR_SSL_WANT_WRITE) continue;
        else if(ret <= 0) return ret;

        slen += ret;

        if(slen >= len) break;
    }

    return slen;
}

/*---------------------------------------------------------------------*/
static int https_read(Http *hi, char *buffer, int len)
{
    if(hi->url.https == 1)
    {
        return mbedtls_ssl_read(&hi->tls.ssl, (u_char *)buffer, (size_t)len);
    }
    else
    {
        return mbedtls_net_recv_timeout(&hi->tls.ssl_fd, (u_char *)buffer, (size_t)len, 5000);
    }
}



bool Http::s_initialized = 0;

void Http::Init()
{
    if( !s_initialized )
    {
#ifdef _WIN32
        WSADATA wsaData;
        int iResult = WSAStartup(MAKEWORD(2,2), &wsaData);
        ASSERT( iResult == 0, "WSAStartup failed" );
#endif

        s_initialized = true;
    }
}

void Http::Close()
{
    if( s_initialized )
    {
#ifdef _WIN32
        int ret = WSACleanup();
        ASSERT(ret == 0);
#endif

        s_initialized = false;
    }
}

Http::Http( bool verify /*= true*/ )
{
    f64 startTime = globalPlatform.CurrentTimeMillis();

    Init();
    https_init( this, 0, verify );

    f64 time = globalPlatform.CurrentTimeMillis();
    fprintf( stdout, "Init: %.1f ms.\n", time - startTime );
}

Http::~Http()
{
    https_close( this );
}


/*---------------------------------------------------------------------*/
bool Http::Open( char const* requestUrl, char* responseOut, int maxResponseLen )
{
    char        err[100];
    char        host[256], port[10], dir[1024];
    int         sock_fd, https, verify;
    int         ret, opt;
    socklen_t   slen;


    f64 startTime = globalPlatform.CurrentTimeMillis();

#if 1
    // TODO Trying to reuse the connection causes the httbin test to fail
    // We must be writing something incorrectly because the read comes back almost immediate with a 400
    url = {};
#endif
    // FIXME We need to pass the lengths here! (probably just pass the final Url struct)
    parse_url(requestUrl, &https, host, port, dir);

    verify = this->tls.verify;

    // FIXME Need to do all this without waiting!
    if ((this->tls.ssl_fd.fd == -1) || (this->url.https != https) ||
        (strcmp(this->url.host, host) != 0) || (strcmp(this->url.port, port) != 0))
    {
        if (this->tls.ssl_fd.fd != -1)
            https_close(this);

        https_init(this, https, verify);

        if ((ret = https_connect(this, host, port)) < 0)
        {
            https_close(this);

            mbedtls_strerror(ret, err, 100);
            snprintf(responseOut, maxResponseLen, "socket error: %s(%d)", err, ret);
            return false;
        }
    }
    else
    {
        sock_fd = this->tls.ssl_fd.fd;

        slen = sizeof(int);

        if ((getsockopt(sock_fd, SOL_SOCKET, SO_ERROR, (char *)&opt, &slen) < 0) || (opt > 0))
        {
            https_close(this);

            https_init(this, https, verify);

            if ((ret = https_connect(this, host, port)) < 0)
            {
                https_close(this);

                mbedtls_strerror(ret, err, 100);
                snprintf(responseOut, maxResponseLen, "socket error: %s(%d)", err, ret);
                return false;
            }
        }
//      else
//          printf("socket reuse: %d \n", sock_fd);
    }

    strcpy(this->url.host, host);
    strcpy(this->url.port, port);
    strcpy(this->url.path, dir);

    f64 time = globalPlatform.CurrentTimeMillis();
    fprintf( stdout, "Open: %.1f ms.\n", time - startTime );

    return true;
}

/*---------------------------------------------------------------------*/
int Http::Get( char const* requestUrl, char *responseOut, int maxResponseLen )
{
    Http::Headers headers;
    return Get( requestUrl, headers, responseOut, maxResponseLen );
}

int Http::Get( char const* requestUrl, Headers& headers, char *responseOut, int maxResponseLen )
{
    bool res = Open( requestUrl, responseOut, maxResponseLen );
    if( !res )
        return 0;

    // Add common and 'mandatory' headers to the user provided set
    String req = BuildRequest( "GET", this->url.host, this->url.port, this->url.path, headers, {} );

    res = Write( req.data, req.length, responseOut, maxResponseLen );
    if( !res )
        return 0;

    this->response.status = 0;
    this->response.contentLength = 0;
    this->response.close = 0;

    this->r_len = 0;
    this->header_end = 0;

    // TODO How do we do this for realz?
    this->body = responseOut;
    this->body_size = maxResponseLen;
    this->body_len = 0;
    this->body[0] = 0;

    return ReadBlocking( responseOut, maxResponseLen );
}

int Http::Post( char const* requestUrl, char const* bodyData, char *responseOut, int maxResponseLen )
{
    Http::Headers headers;
    return Post( requestUrl, headers, bodyData, responseOut, maxResponseLen );
}

int Http::Post( char const* requestUrl, Headers& headers, char const* bodyData, char *responseOut, int maxResponseLen )
{
    bool res = Open( requestUrl, responseOut, maxResponseLen );
    if( !res )
        return 0;

    // Add common and 'mandatory' headers to the user provided set
    String req = BuildRequest( "POST", this->url.host, this->url.port, this->url.path, headers, bodyData );

    res = Write( req.data, req.length, responseOut, maxResponseLen );
    if( !res )
        return 0;

    this->response.status = 0;
    this->response.contentLength = 0;
    this->response.close = 0;

    this->r_len = 0;
    this->header_end = 0;

    // TODO How do we do this for realz?
    this->body = responseOut;
    this->body_size = maxResponseLen;
    this->body_len = 0;
    this->body[0] = 0;

    return ReadBlocking( responseOut, maxResponseLen );
}


String Http::BuildRequest( char const* method, char const* host, char const* port, char const* dir,
                           Headers& headers, String const& bodyData )
{
    // FIXME Store lower-cased, print-out camel-cased
    headers.Put( "User-Agent"_str, "Mozilla/4.0"_str );
    headers.Put( "Host"_str, String::FromFormatTmp( "%s:%s", host, port ) );
    if( bodyData )
        headers.Put( "Content-Length"_str, String::FromFormatTmp( "%d", bodyData.length ) ); 
    headers.PutIfNotFound( "Connection"_str, "Keep-Alive"_str );
    headers.PutIfNotFound( "Accept"_str, "*/*"_str );
    headers.PutIfNotFound( "Content-Type"_str, "application/json; charset=utf-8"_str );


    Data* newReq = (Data*)&request;
    // Retrieve request data from headers
    if( String* h = headers.Get( "Content-Type"_str ) )
        newReq->contentType = String::Clone( *h );
    if( String* h = headers.Get( "Content-Length"_str ) )
    {
        bool ret = h->ToI32( &newReq->contentLength );
        ASSERT( ret );
    }
    if( String* h = headers.Get( "Cookie"_str ) )
        newReq->cookie = String::Clone( *h );
    if( String* h = headers.Get( "Referer"_str ) )
        newReq->referer = String::Clone( *h );
    if( String* h = headers.Get( "Transfer-Encoding"_str ) )
    {
        if( *h == "chunked"_str )
            newReq->chunked = true;
    }
    if( String* h = headers.Get( "Connection"_str ) )
    {
        if( *h == "close"_str )
            newReq->close = true;
    }

    // TODO Do we want to copy this?
#if 0
    newReq->headers = headers;
#endif


    StringBuilder sb;
    sb.AppendFormat( "%s %s HTTP/1.1\r\n", method, dir );
    for( auto it = headers.Items(); it; ++it )
        sb.AppendFormat( "%s: %s\r\n", (*it).key.CStr(), (*it).value.CStr() );
    // TODO Should we skip this when there's no body?
    sb.Append( "\r\n"_str );
    if( bodyData )
        sb.AppendFormat( "%s", bodyData.CStr() );

    String result = sb.ToStringTmp();
#if 0
    printf("--- REQ:\n%s\n---\n", result.CStr() );
#endif

    return result;
}

bool Http::Write( char const* data, int len, char* responseOut, int maxResponseLen )
{
    f64 startTime = globalPlatform.CurrentTimeMillis();

    int ret;
    char err[100];

    if( (ret = https_write(this, data, len)) != len )
    {
        https_close(this);

        mbedtls_strerror( ret, err, sizeof(err) );

        // TODO Store this in a String internal to the class or something, so we don't have to keep passing these around
        snprintf(responseOut, maxResponseLen, "socket error: %s(%d)", err, ret);

        return false;
    }

    f64 time = globalPlatform.CurrentTimeMillis() - startTime;
    fprintf( stdout, "Write: %.1f ms.\n", time );

    return true;
}

/*---------------------------------------------------------------------*/
int Http::WriteChunked( char *data, int len )
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

/*---------------------------------------------------------------------*/
int Http::WriteEnd()
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

/*---------------------------------------------------------------------*/
bool Http::Read( char* responseOut, int maxResponseLen )
{
    int ret;
    char err[100];


    ret = https_read(this, &this->r_buf[this->r_len], (int)(H_READ_SIZE - this->r_len));

    if(ret == MBEDTLS_ERR_SSL_WANT_READ)
        return false;
    else if(ret < 0)
    {
        https_close(this);

        mbedtls_strerror(ret, err, sizeof(err));
        snprintf(responseOut, maxResponseLen, "socket error: %s(%d)", err, ret);

        return true;
    }
    else if(ret == 0)
    {
        https_close(this);
        snprintf( responseOut, maxResponseLen, "Connection closed by peer" );
        return true;
    }

    this->r_len += ret;
    this->r_buf[this->r_len] = 0;

//        printf("read(%ld): %s \n", this->r_len, this->r_buf);
//        printf("read(%ld) \n", this->r_len);

    // TODO Add headers to hashtable
    if( http_parse(this) )
    {
        if(this->response.close == 1)
            https_close(this);

        return true;
    }

    // TODO http_parse seems to fail *sometimes*
    ASSERT( false, "Bad response" );
    return false;
}

int Http::ReadBlocking( char *responseOut, int maxResponseLen )
{
    f64 startTime = globalPlatform.CurrentTimeMillis();

    while(1)
    {
        // TODO Use the logger instead of passing responseOut all the time
        if( Read( responseOut, maxResponseLen ) )
            break;
    }

    f64 time = globalPlatform.CurrentTimeMillis() - startTime;
    fprintf( stdout, "ReadBlocking: %.1f ms.\n", time );

/*
    printf("status: %d \n", this->response.status);
    printf("cookie: %s \n", this->response.cookie);
    printf("location: %s \n", this->response.location);
    printf("referrer: %s \n", this->response.referrer);
    printf("length: %d \n", this->response.content_length);
    printf("body: %d \n", this->body_len);
*/

    return this->response.status;
}

#ifdef _WIN32
#undef strncasecmp

#ifdef _MSC_VER
#pragma warning( pop )
#endif

#endif
