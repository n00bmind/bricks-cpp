@echo off

pushd %~dp0

echo LIBRARY libcrypto-1_1-x64 > libcrypto-1_1-x64.def
echo EXPORTS >> libcrypto-1_1-x64.def
for /f "skip=19 tokens=4" %%A in ('dumpbin /exports libcrypto-1_1-x64.dll') do echo %%A >> libcrypto-1_1-x64.def
lib /def:libcrypto-1_1-x64.def /out:libcrypto-1_1-x64.lib /machine:x64

echo LIBRARY libssl-1_1-x64 > libssl-1_1-x64.def
echo EXPORTS >> libssl-1_1-x64.def
for /f "skip=19 tokens=4" %%A in ('dumpbin /exports libssl-1_1-x64.dll') do echo %%A >> libssl-1_1-x64.def
lib /def:libssl-1_1-x64.def /out:libssl-1_1-x64.lib /machine:x64

popd

