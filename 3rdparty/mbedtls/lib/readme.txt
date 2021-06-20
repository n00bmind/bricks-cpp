Pre-compiled Windows libraries for mbedtls-2.26.0.
Compiled from the mbedtls source distribution (downloaded from github) using Visual Studio 2019.
All compilation options used the defaults provided, except a static runtime (/MT) was specified.

Also, there was a small regression on Windows in 'mbedtls_net_recv_timeout()', as described in https://github.com/ARMmbed/mbedtls/issues/4465. The suggested patch was applied manually.