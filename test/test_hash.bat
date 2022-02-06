@echo off

ctime -begin test_hash.time

cl -nologo -DHASH=%1 -bigobj /Z7 test_hash.cpp

ctime -end test_hash.time

call :setsize test_hash.exe
echo Binary size is %size% bytes
echo.
test_hash.exe

goto :eof

:setsize
set size=%~z1
goto :eof
