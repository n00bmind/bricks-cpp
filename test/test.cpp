
#include "win32.h"
#include <winsock2.h>
#include <DbgHelp.h>

#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <atomic>
#include <mutex>

#define MBEDTLS_ALLOW_PRIVATE_ACCESS    // For accessing 'fd'
#include "mbedtls/net_sockets.h"
#include "mbedtls/entropy.h"
#include "mbedtls/ctr_drbg.h"
#include "mbedtls/error.h"
#if CONFIG_DEBUG
#include "mbedtls/debug.h"
#endif
//#include "mbedtls/../../tests/include/test/certs.h"

#pragma warning( push )
#pragma warning( disable : 4774 )
#pragma warning( disable : 4388 )

#include "gtest/gtest.h"

#pragma warning( pop )


#include "magic.h"
#include "common.h"
#include "intrinsics.h"
#include "maths.h"
#include "platform.h"
#include "clock.h"
#include "memory.h"
#include "context.h"
#include "threading.h"
#include "datatypes.h"
#include "strings.h"
#include "logging.h"
#include "http.h"

#include "common.cpp"
#include "logging.cpp"
#include "http.cpp"
#include "platform.cpp"
#include "win32_platform.cpp"


struct
{
    Http::State http;

} globalState = {};


#undef internal

#pragma warning( push )
#pragma warning( disable : 4355 )
#pragma warning( disable : 4365 )
#pragma warning( disable : 4548 )
#pragma warning( disable : 4530 )
#pragma warning( disable : 4577 )
#pragma warning( disable : 4702 )
#pragma warning( disable : 5219 )


// TODO Math tests

class DatatypesTest : public testing::Test
{
protected:
    // Per-test-suite set-up.
    // Called before the first test in this test suite.
    // Can be omitted if not needed.
    static void SetUpTestCase()
    {
    }

    // Per-test-suite tear-down.
    // Called after the last test in this test suite.
    // Can be omitted if not needed.
    static void TearDownTestCase()
    {
    }

    // Per-test setup/teardown
    virtual void SetUp()
    { }
    virtual void TearDown()
    { }
};

TEST_F( DatatypesTest, ArrayBasics )
{
    Array<int> array( 100 );

    // TODO 
}

TEST_F( DatatypesTest, HashtablePutGet )
{
    persistent LazyAllocator lazyAllocator;
    Hashtable<void*, void*, LazyAllocator> table( 0, &lazyAllocator );

    const int N = 128 * 1024;
    for( int i = 1; i < N; ++i )
        table.Put( (void*)i, (void*)(i + 1) );
    for( int i = 1; i < N; ++i )
        ASSERT_EQ( *table.Get( (void*)i ), (void*)(i + 1) );
}

template <typename DataType>
void TestPushPop()
{
    DataType buffer( 128 );
    ASSERT_EQ( buffer.Count(), 0 );

    for( int i = 0; i < buffer.Capacity(); ++i )
        buffer.Push( i );
    ASSERT_EQ( buffer.Count(), buffer.Capacity() );

    for( int i = 0; i < buffer.Capacity(); ++i )
    {
        int x;
        bool res = buffer.TryPop( &x );
        ASSERT_TRUE( res );
        ASSERT_EQ( x, i );
    }
    ASSERT_EQ( buffer.Count(), 0 );

    for( int i = 0; i < buffer.Capacity(); ++i )
        buffer.Push( i );
    ASSERT_EQ( buffer.Count(), buffer.Capacity() );

    for( int i = 0; i < buffer.Capacity(); ++i )
    {
        int x;
        bool res = buffer.TryPopHead( &x );
        ASSERT_TRUE( res );
        ASSERT_EQ( x, buffer.Capacity() - i - 1 );
    }
    ASSERT_EQ( buffer.Count(), 0 );
}

TEST_F( DatatypesTest, RingBufferBasics )
{
    TestPushPop< RingBuffer<int> >();
    TestPushPop< SyncRingBuffer<int> >();
}

TEST_F( DatatypesTest, SyncQueuePushPop )
{
    SyncQueue<int> q( 128 );

    const int N = 128 * 1024;
    for( int i = 0; i < N; ++i )
        q.Push( i );
    for( int i = 0; i < N; ++i )
    {
        int it;
        bool res = q.TryPop( &it );
        ASSERT_TRUE( res );
        ASSERT_EQ( it, i );
    }
}


template <typename T>
PLATFORM_THREAD_FUNC(MutexTesterThread);
template <typename T>
struct MutexTester
{
    T mutex;
    const int iterationCount;
    const int threadCount;
    i64 value;

    MutexTester( int threadCount_, int iterationCount_ )
        : iterationCount( iterationCount_ )
        , threadCount( threadCount_ )
        , value( 0 )
    {}

    void Test()
    {
        value = 0;

        Array<Platform::ThreadHandle> threads( threadCount );
        for (int i = 0; i < threadCount; i++)
            threads.Push( Core::CreateThread( "Test thread", MutexTesterThread<T>, this, {} ) );
        for( Platform::ThreadHandle& t : threads )
            Core::JoinThread( t );

        ASSERT_EQ( value, threadCount * iterationCount );
    }
};
template <typename T>
PLATFORM_THREAD_FUNC(MutexTesterThread)
{
    MutexTester<T>* tester = (MutexTester<T>*)userdata;

    for( int i = 0; i < tester->iterationCount; i++ )
    {
        tester->mutex.Lock();
        tester->value++;
        tester->mutex.Unlock();
    }
    return 0;
}

TEST( Threading, MutexTest )
{
    MutexTester<Mutex>( 4, 100000 ).Test();

    //MutexTester<Benaphore<PlatformSemaphore>>( 4, 10000 ).Test();
    MutexTester<Benaphore<PreshingSemaphore>>( 4, 100000 ).Test();
    MutexTester<Benaphore<Semaphore>>( 4, 100000 ).Test();

    //MutexTester<RecursiveBenaphore<PlatformSemaphore>>( 4, 10000 ).Test();
    MutexTester<RecursiveBenaphore<PreshingSemaphore>>( 4, 100000 ).Test();
    MutexTester<RecursiveBenaphore<Semaphore>>( 4, 100000 ).Test();
}


// TODO Only do http tests if we detect we're connected. Otherwise show a warning
class HttpTest : public testing::Test
{
protected:
    // Per-test-suite set-up.
    // Called before the first test in this test suite.
    // Can be omitted if not needed.
    static void SetUpTestCase()
    {
    }

    // Per-test-suite tear-down.
    // Called after the last test in this test suite.
    // Can be omitted if not needed.
    static void TearDownTestCase()
    {
    }

    // Per-test setup/teardown
    virtual void SetUp()
    { }
    virtual void TearDown()
    { }
};

TEST_F( HttpTest, Get )
{
    bool done = false;
    auto callback = []( const Http::Response& response, void* userdata  )
    {
        bool* done = (bool*)userdata;
        *done = true;

        ASSERT_EQ( response.statusCode, 200 );
        ASSERT_TRUE( response.body.Valid() );
    };

    u32 ret = Http::Get( &globalState.http, "https://httpbin.org/get?message=https_client", callback, &done );
    ASSERT_TRUE( ret != 0 );

    f32 start = Core::AppTimeSeconds();
    while( !done && (IsDebuggerPresent() || Core::AppTimeSeconds() - start < 10.f) )
        Http::ProcessResponses( &globalState.http );

    ASSERT_TRUE( done );
}

TEST_F( HttpTest, Post )
{
    // TODO Probably abstract/formalize all this
    bool done = false;
    auto callback = []( const Http::Response& response, void* userdata  )
    {
        bool* done = (bool*)userdata;
        *done = true;

        ASSERT_EQ( response.statusCode, 200 );
        ASSERT_TRUE( response.body.Valid() );
    };

    u32 ret = Http::Post( &globalState.http, "https://httpbin.org/post", "{\"message\":\"Hello, https_client!\"}",
                          callback, &done );
    ASSERT_TRUE( ret != 0 );

    f32 start = Core::AppTimeSeconds();
    while( !done && (IsDebuggerPresent() || Core::AppTimeSeconds() - start < 10.f) )
        Http::ProcessResponses( &globalState.http );

    ASSERT_TRUE( done );
}

// TODO Test plain http



// Older Https class/lib

#if 0
// http_open_chunked & http_write_chunked are nowhere to be found

TEST_F( HttpsTest, ChunkedEncoding )
{
    char* url;
    char data[1024], response[4096];
    int  i, ret, size;

    // Init http session. verify: check the server CA cert.
    //Https req( false );
    Https req( true );

    // Test a https post with the chunked-encoding data.
    url = "https://httpbin.org/post";
    ret = http_open_chunked(&req, url);
    if(ret == 0)
    {
        size = snprintf(data, ARRAYCOUNT(data), "[{\"message\":\"Hello, https_client %d\"},", 0);
        int written = http_write_chunked(&req, data, size);
        if(written != size)
        {
            http_strerror(data, 1024);
            ASSERT_EQ( written, size ) << "socket error: " << data;
        }

        for(i=1; i<4; i++)
        {
            size = snprintf(data, ARRAYCOUNT(data), "{\"message\":\"Hello, https_client %d\"},", i);
            written = http_write_chunked(&req, data, size);
            if(written != size)
            {
                http_strerror(data, 1024);
                ASSERT_EQ( written, size ) << "socket error: " << data;
            }
        }

        size = snprintf(data, ARRAYCOUNT(data), "{\"message\":\"Hello, https_client %d\"}]", i);
        written = http_write_chunked(&req, data, strlen(data));
        if(written != size)
        {
            http_strerror(data, 1024);
            ASSERT_EQ( written, size ) << "socket error: " << data;
        }

        ret = http_read_chunked(&req, response, sizeof(response));

        printf("return code: %d \n", ret);
        printf("return body: %s \n", response);
    }
    else
    {
        http_strerror(data, 1024);
        ASSERT_EQ( ret, 0 ) << "socket error: " << data;
    }

    http_close(&req);
}
#endif

// httplib tests
// TODO Incorporate some of these to the new lib's tests
#if 0

#define CPPHTTPLIB_OPENSSL_SUPPORT
#include "httplib.h"


#include <atomic>
#include <chrono>
#include <future>
#include <stdexcept>
#include <thread>
#include <sstream>

#define CA_CERT_FILE "./test/ca-bundle.crt"
#define CLIENT_IMAGE_FILE "./test/image.jpg"

using namespace std;
using namespace httplib;

TEST(ChunkedEncodingTest, FromHTTPWatch) {
    auto host = "www.httpwatch.com";

#ifdef CPPHTTPLIB_OPENSSL_SUPPORT
    auto port = 443;
    SSLClient cli(host, port);
#else
    auto port = 80;
    Client cli(host, port);
#endif
    cli.set_connection_timeout(2);

    auto res =
        cli.Get("/httpgallery/chunked/chunkedimage.aspx?0.4153841143030137");
    ASSERT_TRUE(res);

    std::string out;
    detail::read_file(CLIENT_IMAGE_FILE, out);

    EXPECT_EQ(200, res->status);
    EXPECT_EQ(out, res->body);
}

TEST(ChunkedEncodingTest, WithContentReceiver) {
    auto host = "www.httpwatch.com";

#ifdef CPPHTTPLIB_OPENSSL_SUPPORT
    auto port = 443;
    SSLClient cli(host, port);
#else
    auto port = 80;
    Client cli(host, port);
#endif
    cli.set_connection_timeout(2);

    std::string body;
    auto res =
        cli.Get("/httpgallery/chunked/chunkedimage.aspx?0.4153841143030137",
                [&](const char *data, size_t data_length) {
                body.append(data, data_length);
                return true;
                });
    ASSERT_TRUE(res);

    std::string out;
    detail::read_file(CLIENT_IMAGE_FILE, out);

    EXPECT_EQ(200, res->status);
    EXPECT_EQ(out, body);
}

TEST(ChunkedEncodingTest, WithResponseHandlerAndContentReceiver) {
    auto host = "www.httpwatch.com";

#ifdef CPPHTTPLIB_OPENSSL_SUPPORT
    auto port = 443;
    SSLClient cli(host, port);
#else
    auto port = 80;
    Client cli(host, port);
#endif
    cli.set_connection_timeout(2);

    std::string body;
    auto res = cli.Get(
                       "/httpgallery/chunked/chunkedimage.aspx?0.4153841143030137",
                       [&](const Response &response) {
                       EXPECT_EQ(200, response.status);
                       return true;
                       },
                       [&](const char *data, size_t data_length) {
                       body.append(data, data_length);
                       return true;
                       });
    ASSERT_TRUE(res);

    std::string out;
    detail::read_file(CLIENT_IMAGE_FILE, out);

    EXPECT_EQ(200, res->status);
    EXPECT_EQ(out, body);
}

TEST(AbsoluteRedirectTest, Redirect) {
    auto host = "nghttp2.org";

#ifdef CPPHTTPLIB_OPENSSL_SUPPORT
    SSLClient cli(host);
#else
    Client cli(host);
#endif

    cli.set_follow_location(true);
    auto res = cli.Get("/httpbin/absolute-redirect/3");
    ASSERT_TRUE(res);
    EXPECT_EQ(200, res->status);
}

TEST(RedirectTest, Redirect) {
    auto host = "nghttp2.org";

#ifdef CPPHTTPLIB_OPENSSL_SUPPORT
    SSLClient cli(host);
#else
    Client cli(host);
#endif

    cli.set_follow_location(true);
    auto res = cli.Get("/httpbin/redirect/3");
    ASSERT_TRUE(res);
    EXPECT_EQ(200, res->status);
}

TEST(RelativeRedirectTest, Redirect) {
    auto host = "nghttp2.org";

#ifdef CPPHTTPLIB_OPENSSL_SUPPORT
    SSLClient cli(host);
#else
    Client cli(host);
#endif

    cli.set_follow_location(true);
    auto res = cli.Get("/httpbin/relative-redirect/3");
    ASSERT_TRUE(res);
    EXPECT_EQ(200, res->status);
}

TEST(TooManyRedirectTest, Redirect) {
    auto host = "nghttp2.org";

#ifdef CPPHTTPLIB_OPENSSL_SUPPORT
    SSLClient cli(host);
#else
    Client cli(host);
#endif

    cli.set_follow_location(true);
    auto res = cli.Get("/httpbin/redirect/21");
    ASSERT_TRUE(!res);
    EXPECT_EQ(Error::ExceedRedirectCount, res.error());
}

TEST(HttpsToHttpRedirectTest, Redirect) {
    SSLClient cli("nghttp2.org");
    cli.set_follow_location(true);
    auto res = cli.Get(
                       "/httpbin/redirect-to?url=http%3A%2F%2Fwww.google.com&status_code=302");
    ASSERT_TRUE(res);
    EXPECT_EQ(200, res->status);
}

TEST(HttpsToHttpRedirectTest2, Redirect) {
    SSLClient cli("nghttp2.org");
    cli.set_follow_location(true);

    Params params;
    params.emplace("url", "http://www.google.com");
    params.emplace("status_code", "302");

    auto res = cli.Get("/httpbin/redirect-to", params, Headers{});
    ASSERT_TRUE(res);
    EXPECT_EQ(200, res->status);
}

TEST(HttpsToHttpRedirectTest3, Redirect) {
    SSLClient cli("nghttp2.org");
    cli.set_follow_location(true);

    Params params;
    params.emplace("url", "http://www.google.com");

    auto res = cli.Get("/httpbin/redirect-to?status_code=302", params, Headers{});
    ASSERT_TRUE(res);
    EXPECT_EQ(200, res->status);
}

TEST(HttpsToHttpRedirectTest, SimpleInterface) {
    Client cli("https://nghttp2.org");
    cli.set_follow_location(true);
    auto res =
        cli.Get("/httpbin/"
                "redirect-to?url=http%3A%2F%2Fwww.google.com&status_code=302");

    ASSERT_TRUE(res);
    EXPECT_EQ(200, res->status);
}

TEST(HttpsToHttpRedirectTest2, SimpleInterface) {
    Client cli("https://nghttp2.org");
    cli.set_follow_location(true);

    Params params;
    params.emplace("url", "http://www.google.com");
    params.emplace("status_code", "302");

    auto res = cli.Get("/httpbin/redirect-to", params, Headers{});
    ASSERT_TRUE(res);
    EXPECT_EQ(200, res->status);
}

TEST(HttpsToHttpRedirectTest3, SimpleInterface) {
    Client cli("https://nghttp2.org");
    cli.set_follow_location(true);

    Params params;
    params.emplace("url", "http://www.google.com");

    auto res = cli.Get("/httpbin/redirect-to?status_code=302", params, Headers{});
    ASSERT_TRUE(res);
    EXPECT_EQ(200, res->status);
}

// httplib tests
#endif


#include "gtest/gtest-all.cc"

#pragma warning( pop )


GTEST_API_ int main(int argc, char **argv)
{
#if 1
    SetUnhandledExceptionFilter( Win32::ExceptionHandler );
#endif

    Logging::ChannelDecl channels[] =
    {
        { "Platform" },
        { "Net" },
    };
    InitGlobalPlatform( channels );

    bool result = Http::Init( &globalState.http );
    ASSERT_EQ( result, true ) << "Http::Init failed";

    testing::InitGoogleTest(&argc, argv);
    int testResult = RUN_ALL_TESTS();

    Http::Shutdown( &globalState.http );
    return testResult;
}

