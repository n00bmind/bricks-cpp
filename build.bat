cl /I3rdparty\openssl\include main.cpp 3rdparty\openssl\libcrypto-1_1-x64.lib 3rdparty\openssl\libssl-1_1-x64.lib

cl /I3rdparty\openssl\include test\test.cc 3rdparty\openssl\libcrypto-1_1-x64.lib 3rdparty\openssl\libssl-1_1-x64.lib
