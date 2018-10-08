@echo off
set THIS_DIR=%~sdp0

copy /y %THIS_DIR%..\..\build\windows-qt4.8.7\build-mbedtls-Desktop-Debug\debug\libmbedtls.a   %THIS_DIR%windows-qt4.8.7\
copy /y %THIS_DIR%..\..\build\windows-qt4.8.7\build-pro_util-Desktop-Debug\debug\libpro_util.a %THIS_DIR%windows-qt4.8.7\

pause
