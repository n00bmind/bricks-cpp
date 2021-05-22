#define CPPHTTPLIB_OPENSSL_SUPPORT

#include "../httplib.h"

#include "gtest/gtest.h"
#include "gtest/gtest-all.cc"

#include <atomic>
#include <chrono>
#include <future>
#include <stdexcept>
#include <thread>
#include <sstream>

#define SERVER_CERT_FILE "./cert.pem"
#define SERVER_CERT2_FILE "./cert2.pem"
#define SERVER_PRIVATE_KEY_FILE "./key.pem"
#define CA_CERT_FILE "./ca-bundle.crt"
#define CLIENT_CA_CERT_FILE "./rootCA.cert.pem"
#define CLIENT_CA_CERT_DIR "."
#define CLIENT_CERT_FILE "./client.cert.pem"
#define CLIENT_PRIVATE_KEY_FILE "./client.key.pem"

using namespace std;
using namespace httplib;

const char *HOST = "localhost";
const int PORT = 1234;

const string LONG_QUERY_VALUE = string(25000, '@');
const string LONG_QUERY_URL = "/long-query-value?key=" + LONG_QUERY_VALUE;

const std::string JSON_DATA = "{\"hello\":\"world\"}";

const string LARGE_DATA = string(1024 * 1024 * 100, '@'); // 100MB

MultipartFormData &get_file_value(MultipartFormDataItems &files,
                                  const char *key) {
    auto it = std::find_if(
                           files.begin(), files.end(),
                           [&](const MultipartFormData &file) { return file.name == key; });
    if (it != files.end()) { return *it; }
    throw std::runtime_error("invalid mulitpart form data name error");
}

#ifdef _WIN32
TEST(StartupTest, WSAStartup) {
    WSADATA wsaData;
    int ret = WSAStartup(0x0002, &wsaData);
    ASSERT_EQ(0, ret);
}
#endif

TEST(EncodeQueryParamTest, ParseUnescapedChararactersTest) {
    string unescapedCharacters = "-_.!~*'()";

    EXPECT_EQ(detail::encode_query_param(unescapedCharacters), "-_.!~*'()");
}

TEST(EncodeQueryParamTest, ParseReservedCharactersTest) {
    string reservedCharacters = ";,/?:@&=+$";

    EXPECT_EQ(detail::encode_query_param(reservedCharacters),
              "%3B%2C%2F%3F%3A%40%26%3D%2B%24");
}

TEST(EncodeQueryParamTest, TestUTF8Characters) {
    string chineseCharacters = "中国語";
    string russianCharacters = "дом";
    string brazilianCharacters = "óculos";

    EXPECT_EQ(detail::encode_query_param(chineseCharacters),
              "%E4%B8%AD%E5%9B%BD%E8%AA%9E");

    EXPECT_EQ(detail::encode_query_param(russianCharacters),
              "%D0%B4%D0%BE%D0%BC");

    EXPECT_EQ(detail::encode_query_param(brazilianCharacters), "%C3%B3culos");
}

TEST(TrimTests, TrimStringTests) {
    EXPECT_EQ("abc", detail::trim_copy("abc"));
    EXPECT_EQ("abc", detail::trim_copy("  abc  "));
    EXPECT_TRUE(detail::trim_copy("").empty());
}

TEST(SplitTest, ParseQueryString) {
    string s = "key1=val1&key2=val2&key3=val3";
    Params dic;

    detail::split(s.c_str(), s.c_str() + s.size(), '&',
                  [&](const char *b, const char *e) {
                  string key, val;
                  detail::split(b, e, '=', [&](const char *b, const char *e) {
                                if (key.empty()) {
                                key.assign(b, e);
                                } else {
                                val.assign(b, e);
                                }
                                });
                  dic.emplace(key, val);
                  });

    EXPECT_EQ("val1", dic.find("key1")->second);
    EXPECT_EQ("val2", dic.find("key2")->second);
    EXPECT_EQ("val3", dic.find("key3")->second);
}

TEST(SplitTest, ParseInvalidQueryTests) {

    {
        string s = " ";
        Params dict;
        detail::parse_query_text(s, dict);
        EXPECT_TRUE(dict.empty());
    }

    {
        string s = " = =";
        Params dict;
        detail::parse_query_text(s, dict);
        EXPECT_TRUE(dict.empty());
    }
}

TEST(ParseQueryTest, ParseQueryString) {
    string s = "key1=val1&key2=val2&key3=val3";
    Params dic;

    detail::parse_query_text(s, dic);

    EXPECT_EQ("val1", dic.find("key1")->second);
    EXPECT_EQ("val2", dic.find("key2")->second);
    EXPECT_EQ("val3", dic.find("key3")->second);
}

TEST(ParamsToQueryTest, ConvertParamsToQuery) {
    Params dic;

    EXPECT_EQ(detail::params_to_query_str(dic), "");

    dic.emplace("key1", "val1");

    EXPECT_EQ(detail::params_to_query_str(dic), "key1=val1");

    dic.emplace("key2", "val2");
    dic.emplace("key3", "val3");

    EXPECT_EQ(detail::params_to_query_str(dic), "key1=val1&key2=val2&key3=val3");
}

TEST(GetHeaderValueTest, DefaultValue) {
    Headers headers = {{"Dummy", "Dummy"}};
    auto val = detail::get_header_value(headers, "Content-Type", 0, "text/plain");
    EXPECT_STREQ("text/plain", val);
}

TEST(GetHeaderValueTest, DefaultValueInt) {
    Headers headers = {{"Dummy", "Dummy"}};
    auto val =
        detail::get_header_value<uint64_t>(headers, "Content-Length", 0, 100);
    EXPECT_EQ(100ull, val);
}

TEST(GetHeaderValueTest, RegularValue) {
    Headers headers = {{"Content-Type", "text/html"}, {"Dummy", "Dummy"}};
    auto val = detail::get_header_value(headers, "Content-Type", 0, "text/plain");
    EXPECT_STREQ("text/html", val);
}

TEST(GetHeaderValueTest, SetContent) {
    Response res;

    res.set_content("html", "text/html");
    EXPECT_EQ("text/html", res.get_header_value("Content-Type"));

    res.set_content("text", "text/plain");
    EXPECT_EQ(1, res.get_header_value_count("Content-Type"));
    EXPECT_EQ("text/plain", res.get_header_value("Content-Type"));
}

TEST(GetHeaderValueTest, RegularValueInt) {
    Headers headers = {{"Content-Length", "100"}, {"Dummy", "Dummy"}};
    auto val =
        detail::get_header_value<uint64_t>(headers, "Content-Length", 0, 0);
    EXPECT_EQ(100ull, val);
}

TEST(GetHeaderValueTest, Range) {
    {
        Headers headers = {make_range_header({{1, -1}})};
        auto val = detail::get_header_value(headers, "Range", 0, 0);
        EXPECT_STREQ("bytes=1-", val);
    }

    {
        Headers headers = {make_range_header({{-1, 1}})};
        auto val = detail::get_header_value(headers, "Range", 0, 0);
        EXPECT_STREQ("bytes=-1", val);
    }

    {
        Headers headers = {make_range_header({{1, 10}})};
        auto val = detail::get_header_value(headers, "Range", 0, 0);
        EXPECT_STREQ("bytes=1-10", val);
    }

    {
        Headers headers = {make_range_header({{1, 10}, {100, -1}})};
        auto val = detail::get_header_value(headers, "Range", 0, 0);
        EXPECT_STREQ("bytes=1-10, 100-", val);
    }

    {
        Headers headers = {make_range_header({{1, 10}, {100, 200}})};
        auto val = detail::get_header_value(headers, "Range", 0, 0);
        EXPECT_STREQ("bytes=1-10, 100-200", val);
    }

    {
        Headers headers = {make_range_header({{0, 0}, {-1, 1}})};
        auto val = detail::get_header_value(headers, "Range", 0, 0);
        EXPECT_STREQ("bytes=0-0, -1", val);
    }
}

TEST(ParseHeaderValueTest, Range) {
    {
        Ranges ranges;
        auto ret = detail::parse_range_header("bytes=1-", ranges);
        EXPECT_TRUE(ret);
        EXPECT_EQ(1u, ranges.size());
        EXPECT_EQ(1u, ranges[0].first);
        EXPECT_EQ(-1, ranges[0].second);
    }

    {
        Ranges ranges;
        auto ret = detail::parse_range_header("bytes=-1", ranges);
        EXPECT_TRUE(ret);
        EXPECT_EQ(1u, ranges.size());
        EXPECT_EQ(-1, ranges[0].first);
        EXPECT_EQ(1u, ranges[0].second);
    }

    {
        Ranges ranges;
        auto ret = detail::parse_range_header("bytes=1-10", ranges);
        EXPECT_TRUE(ret);
        EXPECT_EQ(1u, ranges.size());
        EXPECT_EQ(1u, ranges[0].first);
        EXPECT_EQ(10u, ranges[0].second);
    }

    {
        Ranges ranges;
        auto ret = detail::parse_range_header("bytes=10-1", ranges);
        EXPECT_FALSE(ret);
    }

    {
        Ranges ranges;
        auto ret = detail::parse_range_header("bytes=1-10, 100-", ranges);
        EXPECT_TRUE(ret);
        EXPECT_EQ(2u, ranges.size());
        EXPECT_EQ(1u, ranges[0].first);
        EXPECT_EQ(10u, ranges[0].second);
        EXPECT_EQ(100u, ranges[1].first);
        EXPECT_EQ(-1, ranges[1].second);
    }

    {
        Ranges ranges;
        auto ret =
            detail::parse_range_header("bytes=1-10, 100-200, 300-400", ranges);
        EXPECT_TRUE(ret);
        EXPECT_EQ(3u, ranges.size());
        EXPECT_EQ(1u, ranges[0].first);
        EXPECT_EQ(10u, ranges[0].second);
        EXPECT_EQ(100u, ranges[1].first);
        EXPECT_EQ(200u, ranges[1].second);
        EXPECT_EQ(300u, ranges[2].first);
        EXPECT_EQ(400u, ranges[2].second);
    }
}

TEST(ParseAcceptEncoding1, AcceptEncoding) {
    Request req;
    req.set_header("Accept-Encoding", "gzip");

    Response res;
    res.set_header("Content-Type", "text/plain");

    auto ret = detail::encoding_type(req, res);

#ifdef CPPHTTPLIB_ZLIB_SUPPORT
    EXPECT_TRUE(ret == detail::EncodingType::Gzip);
#else
    EXPECT_TRUE(ret == detail::EncodingType::None);
#endif
}

TEST(ParseAcceptEncoding2, AcceptEncoding) {
    Request req;
    req.set_header("Accept-Encoding", "gzip, deflate, br");

    Response res;
    res.set_header("Content-Type", "text/plain");

    auto ret = detail::encoding_type(req, res);

#ifdef CPPHTTPLIB_BROTLI_SUPPORT
    EXPECT_TRUE(ret == detail::EncodingType::Brotli);
#elif CPPHTTPLIB_ZLIB_SUPPORT
    EXPECT_TRUE(ret == detail::EncodingType::Gzip);
#else
    EXPECT_TRUE(ret == detail::EncodingType::None);
#endif
}

TEST(ParseAcceptEncoding3, AcceptEncoding) {
    Request req;
    req.set_header("Accept-Encoding", "br;q=1.0, gzip;q=0.8, *;q=0.1");

    Response res;
    res.set_header("Content-Type", "text/plain");

    auto ret = detail::encoding_type(req, res);

#ifdef CPPHTTPLIB_BROTLI_SUPPORT
    EXPECT_TRUE(ret == detail::EncodingType::Brotli);
#elif CPPHTTPLIB_ZLIB_SUPPORT
    EXPECT_TRUE(ret == detail::EncodingType::Gzip);
#else
    EXPECT_TRUE(ret == detail::EncodingType::None);
#endif
}

TEST(BufferStreamTest, read) {
    detail::BufferStream strm1;
    Stream &strm = strm1;

    EXPECT_EQ(5, strm.write("hello"));

    char buf[512];
    EXPECT_EQ(2, strm.read(buf, 2));
    EXPECT_EQ('h', buf[0]);
    EXPECT_EQ('e', buf[1]);

    EXPECT_EQ(2, strm.read(buf, 2));
    EXPECT_EQ('l', buf[0]);
    EXPECT_EQ('l', buf[1]);

    EXPECT_EQ(1, strm.read(buf, 1));
    EXPECT_EQ('o', buf[0]);

    EXPECT_EQ(0, strm.read(buf, 1));
}

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
    detail::read_file("./image.jpg", out);

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
    detail::read_file("./image.jpg", out);

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
    detail::read_file("./image.jpg", out);

    EXPECT_EQ(200, res->status);
    EXPECT_EQ(out, body);
}

TEST(DefaultHeadersTest, FromHTTPBin) {
    Client cli("httpbin.org");
    cli.set_default_headers({make_range_header({{1, 10}})});
    cli.set_connection_timeout(5);

    {
        auto res = cli.Get("/range/32");
        ASSERT_TRUE(res);
        EXPECT_EQ("bcdefghijk", res->body);
        EXPECT_EQ(206, res->status);
    }

    {
        auto res = cli.Get("/range/32");
        ASSERT_TRUE(res);
        EXPECT_EQ("bcdefghijk", res->body);
        EXPECT_EQ(206, res->status);
    }
}

TEST(RangeTest, FromHTTPBin) {
    auto host = "httpbin.org";

#ifdef CPPHTTPLIB_OPENSSL_SUPPORT
    auto port = 443;
    SSLClient cli(host, port);
#else
    auto port = 80;
    Client cli(host, port);
#endif
    cli.set_connection_timeout(5);

    {
        auto res = cli.Get("/range/32");
        ASSERT_TRUE(res);
        EXPECT_EQ("abcdefghijklmnopqrstuvwxyzabcdef", res->body);
        EXPECT_EQ(200, res->status);
    }

    {
        Headers headers = {make_range_header({{1, -1}})};
        auto res = cli.Get("/range/32", headers);
        ASSERT_TRUE(res);
        EXPECT_EQ("bcdefghijklmnopqrstuvwxyzabcdef", res->body);
        EXPECT_EQ(206, res->status);
    }

    {
        Headers headers = {make_range_header({{1, 10}})};
        auto res = cli.Get("/range/32", headers);
        ASSERT_TRUE(res);
        EXPECT_EQ("bcdefghijk", res->body);
        EXPECT_EQ(206, res->status);
    }

    {
        Headers headers = {make_range_header({{0, 31}})};
        auto res = cli.Get("/range/32", headers);
        ASSERT_TRUE(res);
        EXPECT_EQ("abcdefghijklmnopqrstuvwxyzabcdef", res->body);
        EXPECT_EQ(200, res->status);
    }

    {
        Headers headers = {make_range_header({{0, -1}})};
        auto res = cli.Get("/range/32", headers);
        ASSERT_TRUE(res);
        EXPECT_EQ("abcdefghijklmnopqrstuvwxyzabcdef", res->body);
        EXPECT_EQ(200, res->status);
    }

    {
        Headers headers = {make_range_header({{0, 32}})};
        auto res = cli.Get("/range/32", headers);
        ASSERT_TRUE(res);
        EXPECT_EQ(416, res->status);
    }
}

TEST(ConnectionErrorTest, InvalidHost) {
    auto host = "-abcde.com";

#ifdef CPPHTTPLIB_OPENSSL_SUPPORT
    auto port = 443;
    SSLClient cli(host, port);
#else
    auto port = 80;
    Client cli(host, port);
#endif
    cli.set_connection_timeout(std::chrono::seconds(2));

    auto res = cli.Get("/");
    ASSERT_TRUE(!res);
    EXPECT_EQ(Error::Connection, res.error());
}

TEST(ConnectionErrorTest, InvalidHost2) {
    auto host = "httpbin.org/";

#ifdef CPPHTTPLIB_OPENSSL_SUPPORT
    SSLClient cli(host);
#else
    Client cli(host);
#endif
    cli.set_connection_timeout(std::chrono::seconds(2));

    auto res = cli.Get("/");
    ASSERT_TRUE(!res);
    EXPECT_EQ(Error::Connection, res.error());
}

TEST(ConnectionErrorTest, InvalidHostCheckResultErrorToString) {
    auto host = "httpbin.org/";

#ifdef CPPHTTPLIB_OPENSSL_SUPPORT
    SSLClient cli(host);
#else
    Client cli(host);
#endif
    cli.set_connection_timeout(std::chrono::seconds(2));

    auto res = cli.Get("/");
    ASSERT_TRUE(!res);
    stringstream s;
    s << "error code: " << res.error();
    EXPECT_EQ("error code: 2", s.str());
}

TEST(ConnectionErrorTest, InvalidPort) {
    auto host = "localhost";
    auto port = 44380;

#ifdef CPPHTTPLIB_OPENSSL_SUPPORT
    SSLClient cli(host, port);
#else
    Client cli(host, port);
#endif
    cli.set_connection_timeout(std::chrono::seconds(2));

    auto res = cli.Get("/");
    ASSERT_TRUE(!res);
    EXPECT_EQ(Error::Connection, res.error());
}

TEST(ConnectionErrorTest, Timeout) {
    auto host = "google.com";

#ifdef CPPHTTPLIB_OPENSSL_SUPPORT
    auto port = 44380;
    SSLClient cli(host, port);
#else
    auto port = 8080;
    Client cli(host, port);
#endif
    cli.set_connection_timeout(std::chrono::seconds(2));

    auto res = cli.Get("/");
    ASSERT_TRUE(!res);
    EXPECT_TRUE(res.error() == Error::Connection);
}

TEST(CancelTest, NoCancel) {
    auto host = "httpbin.org";

#ifdef CPPHTTPLIB_OPENSSL_SUPPORT
    auto port = 443;
    SSLClient cli(host, port);
#else
    auto port = 80;
    Client cli(host, port);
#endif
    cli.set_connection_timeout(std::chrono::seconds(5));

    auto res = cli.Get("/range/32", [](uint64_t, uint64_t) { return true; });
    ASSERT_TRUE(res);
    EXPECT_EQ("abcdefghijklmnopqrstuvwxyzabcdef", res->body);
    EXPECT_EQ(200, res->status);
}

TEST(CancelTest, WithCancelSmallPayload) {
    auto host = "httpbin.org";

#ifdef CPPHTTPLIB_OPENSSL_SUPPORT
    auto port = 443;
    SSLClient cli(host, port);
#else
    auto port = 80;
    Client cli(host, port);
#endif

    auto res = cli.Get("/range/32", [](uint64_t, uint64_t) { return false; });
    cli.set_connection_timeout(std::chrono::seconds(5));
    ASSERT_TRUE(!res);
    EXPECT_EQ(Error::Canceled, res.error());
}

TEST(CancelTest, WithCancelLargePayload) {
    auto host = "httpbin.org";

#ifdef CPPHTTPLIB_OPENSSL_SUPPORT
    auto port = 443;
    SSLClient cli(host, port);
#else
    auto port = 80;
    Client cli(host, port);
#endif
    cli.set_connection_timeout(std::chrono::seconds(5));

    uint32_t count = 0;
    auto res = cli.Get("/range/65536",
                       [&count](uint64_t, uint64_t) { return (count++ == 0); });
    ASSERT_TRUE(!res);
    EXPECT_EQ(Error::Canceled, res.error());
}

TEST(BaseAuthTest, FromHTTPWatch) {
    auto host = "httpbin.org";

#ifdef CPPHTTPLIB_OPENSSL_SUPPORT
    auto port = 443;
    SSLClient cli(host, port);
#else
    auto port = 80;
    Client cli(host, port);
#endif

    {
        auto res = cli.Get("/basic-auth/hello/world");
        ASSERT_TRUE(res);
        EXPECT_EQ(401, res->status);
    }

    {
        auto res = cli.Get("/basic-auth/hello/world",
                           {make_basic_authentication_header("hello", "world")});
        ASSERT_TRUE(res);
        EXPECT_EQ("{\n  \"authenticated\": true, \n  \"user\": \"hello\"\n}\n",
                  res->body);
        EXPECT_EQ(200, res->status);
    }

    {
        cli.set_basic_auth("hello", "world");
        auto res = cli.Get("/basic-auth/hello/world");
        ASSERT_TRUE(res);
        EXPECT_EQ("{\n  \"authenticated\": true, \n  \"user\": \"hello\"\n}\n",
                  res->body);
        EXPECT_EQ(200, res->status);
    }

    {
        cli.set_basic_auth("hello", "bad");
        auto res = cli.Get("/basic-auth/hello/world");
        ASSERT_TRUE(res);
        EXPECT_EQ(401, res->status);
    }

    {
        cli.set_basic_auth("bad", "world");
        auto res = cli.Get("/basic-auth/hello/world");
        ASSERT_TRUE(res);
        EXPECT_EQ(401, res->status);
    }
}

#ifdef CPPHTTPLIB_OPENSSL_SUPPORT
TEST(DigestAuthTest, FromHTTPWatch) {
    auto host = "httpbin.org";
    auto port = 443;
    SSLClient cli(host, port);

    {
        auto res = cli.Get("/digest-auth/auth/hello/world");
        ASSERT_TRUE(res);
        EXPECT_EQ(401, res->status);
    }

    {
        std::vector<std::string> paths = {
            "/digest-auth/auth/hello/world/MD5",
            "/digest-auth/auth/hello/world/SHA-256",
            "/digest-auth/auth/hello/world/SHA-512",
            "/digest-auth/auth-int/hello/world/MD5",
        };

        cli.set_digest_auth("hello", "world");
        for (auto path : paths) {
            auto res = cli.Get(path.c_str());
            ASSERT_TRUE(res);
            EXPECT_EQ("{\n  \"authenticated\": true, \n  \"user\": \"hello\"\n}\n",
                      res->body);
            EXPECT_EQ(200, res->status);
        }

        cli.set_digest_auth("hello", "bad");
        for (auto path : paths) {
            auto res = cli.Get(path.c_str());
            ASSERT_TRUE(res);
            EXPECT_EQ(401, res->status);
        }

        // NOTE: Until httpbin.org fixes issue #46, the following test is commented
        // out. Plese see https://httpbin.org/digest-auth/auth/hello/world
        // cli.set_digest_auth("bad", "world");
        // for (auto path : paths) {
        //   auto res = cli.Get(path.c_str());
        //   ASSERT_TRUE(res);
        //   EXPECT_EQ(400, res->status);
        // }
    }
}
#endif

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

#ifdef CPPHTTPLIB_OPENSSL_SUPPORT
TEST(YahooRedirectTest, Redirect) {
    Client cli("yahoo.com");

    auto res = cli.Get("/");
    ASSERT_TRUE(res);
    EXPECT_EQ(301, res->status);

    cli.set_follow_location(true);
    res = cli.Get("/");
    ASSERT_TRUE(res);
    EXPECT_EQ(200, res->status);
    EXPECT_EQ("https://yahoo.com/", res->location);
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

TEST(UrlWithSpace, Redirect) {
    SSLClient cli("edge.forgecdn.net");
    cli.set_follow_location(true);

    auto res = cli.Get("/files/2595/310/Neat 1.4-17.jar");
    ASSERT_TRUE(res);
    EXPECT_EQ(200, res->status);
    EXPECT_EQ(18527, res->get_header_value<uint64_t>("Content-Length"));
}

#endif



// Sends a raw request to a server listening at HOST:PORT.
static bool send_request(time_t read_timeout_sec, const std::string &req,
                         std::string *resp = nullptr) {
    auto error = Error::Success;

    auto client_sock =
        detail::create_client_socket(HOST, PORT, AF_UNSPEC, false, nullptr,
                                     /*connection_timeout_sec=*/5, 0,
                                     /*read_timeout_sec=*/5, 0,
                                     /*write_timeout_sec=*/5, 0,
                                     std::string(), error);

    if (client_sock == INVALID_SOCKET) { return false; }

    auto ret = detail::process_client_socket(
                                             client_sock, read_timeout_sec, 0, 0, 0, [&](Stream &strm) {
                                             if (req.size() !=
                                                 static_cast<size_t>(strm.write(req.data(), req.size()))) {
                                             return false;
                                             }

                                             char buf[512];

                                             detail::stream_line_reader line_reader(strm, buf, sizeof(buf));
                                             while (line_reader.getline()) {
                                             if (resp) { *resp += line_reader.ptr(); }
                                             }
                                             return true;
                                             });

    detail::close_socket(client_sock);

    return ret;
}

#ifdef CPPHTTPLIB_OPENSSL_SUPPORT

TEST(SSLClientTest, UpdateCAStore) {
    httplib::SSLClient httplib_client("www.google.com");
    auto ca_store_1 = X509_STORE_new();
    X509_STORE_load_locations(ca_store_1, "/etc/ssl/certs/ca-certificates.crt",
                              nullptr);
    httplib_client.set_ca_cert_store(ca_store_1);

    auto ca_store_2 = X509_STORE_new();
    X509_STORE_load_locations(ca_store_2, "/etc/ssl/certs/ca-certificates.crt",
                              nullptr);
    httplib_client.set_ca_cert_store(ca_store_2);
}

TEST(SSLClientTest, ServerNameIndication) {
    SSLClient cli("httpbin.org", 443);
    auto res = cli.Get("/get");
    ASSERT_TRUE(res);
    ASSERT_EQ(200, res->status);
}

TEST(SSLClientTest, ServerCertificateVerification1) {
    SSLClient cli("google.com");
    auto res = cli.Get("/");
    ASSERT_TRUE(res);
    ASSERT_EQ(301, res->status);
}

TEST(SSLClientTest, ServerCertificateVerification2) {
    SSLClient cli("google.com");
    cli.enable_server_certificate_verification(true);
    cli.set_ca_cert_path("hello");
    auto res = cli.Get("/");
    ASSERT_TRUE(!res);
    EXPECT_EQ(Error::SSLLoadingCerts, res.error());
}

TEST(SSLClientTest, ServerCertificateVerification3) {
    SSLClient cli("google.com");
    cli.set_ca_cert_path(CA_CERT_FILE);
    auto res = cli.Get("/");
    ASSERT_TRUE(res);
    ASSERT_EQ(301, res->status);
}

TEST(SSLClientTest, WildcardHostNameMatch) {
    SSLClient cli("www.youtube.com");

    cli.set_ca_cert_path(CA_CERT_FILE);
    cli.enable_server_certificate_verification(true);
    cli.set_follow_location(true);

    auto res = cli.Get("/");
    ASSERT_TRUE(res);
    ASSERT_EQ(200, res->status);
}

#endif

#ifdef _WIN32
TEST(CleanupTest, WSACleanup) {
    int ret = WSACleanup();
    ASSERT_EQ(0, ret);
}
#endif

#ifndef CPPHTTPLIB_OPENSSL_SUPPORT
TEST(NoSSLSupport, SimpleInterface) {
    ASSERT_ANY_THROW(Client cli("https://yahoo.com"));
}
#endif

TEST(InvalidScheme, SimpleInterface) {
    ASSERT_ANY_THROW(Client cli("scheme://yahoo.com"));
}

TEST(NoScheme, SimpleInterface) {
    Client cli("yahoo.com:80");
    ASSERT_TRUE(cli.is_valid());
}

TEST(SendAPI, SimpleInterface) {
    Client cli("http://yahoo.com");

    Request req;
    req.method = "GET";
    req.path = "/";
    auto res = cli.send(req);

    ASSERT_TRUE(res);
    EXPECT_EQ(301, res->status);
}

#ifdef CPPHTTPLIB_OPENSSL_SUPPORT
TEST(YahooRedirectTest2, SimpleInterface) {
    Client cli("http://yahoo.com");

    auto res = cli.Get("/");
    ASSERT_TRUE(res);
    EXPECT_EQ(301, res->status);

    cli.set_follow_location(true);
    res = cli.Get("/");
    ASSERT_TRUE(res);
    EXPECT_EQ(200, res->status);
    EXPECT_EQ("https://yahoo.com/", res->location);
}

TEST(YahooRedirectTest3, SimpleInterface) {
    Client cli("https://yahoo.com");

    auto res = cli.Get("/");
    ASSERT_TRUE(res);
    EXPECT_EQ(301, res->status);

    cli.set_follow_location(true);
    res = cli.Get("/");
    ASSERT_TRUE(res);
    EXPECT_EQ(200, res->status);
    EXPECT_EQ("https://www.yahoo.com/", res->location);
}

TEST(YahooRedirectTest3, NewResultInterface) {
    Client cli("https://yahoo.com");

    auto res = cli.Get("/");
    ASSERT_TRUE(res);
    ASSERT_FALSE(!res);
    ASSERT_TRUE(res);
    ASSERT_FALSE(res == nullptr);
    ASSERT_TRUE(res != nullptr);
    EXPECT_EQ(Error::Success, res.error());
    EXPECT_EQ(301, res.value().status);
    EXPECT_EQ(301, (*res).status);
    EXPECT_EQ(301, res->status);

    cli.set_follow_location(true);
    res = cli.Get("/");
    ASSERT_TRUE(res);
    EXPECT_EQ(Error::Success, res.error());
    EXPECT_EQ(200, res.value().status);
    EXPECT_EQ(200, (*res).status);
    EXPECT_EQ(200, res->status);
    EXPECT_EQ("https://www.yahoo.com/", res->location);
}

#ifdef CPPHTTPLIB_BROTLI_SUPPORT
TEST(DecodeWithChunkedEncoding, BrotliEncoding) {
    Client cli("https://cdnjs.cloudflare.com");
    auto res =
        cli.Get("/ajax/libs/jquery/3.5.1/jquery.js", {{"Accept-Encoding", "br"}});

    ASSERT_TRUE(res);
    EXPECT_EQ(200, res->status);
    EXPECT_EQ(287630, res->body.size());
    EXPECT_EQ("application/javascript; charset=utf-8",
              res->get_header_value("Content-Type"));
}
#endif

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
#endif




GTEST_API_ int main(int argc, char **argv) {
    testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}

