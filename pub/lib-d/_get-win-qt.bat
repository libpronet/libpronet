@echo off
set THIS_DIR=%~sdp0

copy /y %THIS_DIR%..\..\build\windows-qt487-gcc482\build-mbedtls-Desktop-Debug\debug\libmbedtls.a   %THIS_DIR%windows-qt487-gcc482\x86\
copy /y %THIS_DIR%..\..\build\windows-qt487-gcc482\build-pro_util-Desktop-Debug\debug\libpro_util.a %THIS_DIR%windows-qt487-gcc482\x86\

pause
