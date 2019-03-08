@echo off
set THIS_DIR=%~sdp0

copy /y %THIS_DIR%..\..\build\windows-qt487-gcc482\build-mbedtls-Desktop-Release\release\libmbedtls.a   %THIS_DIR%windows-qt487-gcc482\x86\
copy /y %THIS_DIR%..\..\build\windows-qt487-gcc482\build-pro_util-Desktop-Release\release\libpro_util.a %THIS_DIR%windows-qt487-gcc482\x86\

pause
