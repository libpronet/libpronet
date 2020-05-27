@echo off
set THIS_DIR=%~sdp0

if exist %THIS_DIR%libs\armeabi-v7a      rmdir /s /q %THIS_DIR%libs\armeabi-v7a
if exist %THIS_DIR%libs\x86              rmdir /s /q %THIS_DIR%libs\x86
if exist %THIS_DIR%obj\local\armeabi-v7a rmdir /s /q %THIS_DIR%obj\local\armeabi-v7a
if exist %THIS_DIR%obj\local\x86         rmdir /s /q %THIS_DIR%obj\local\x86

@echo on
call ndk-build -B V=1 APP_ABI="armeabi-v7a x86"
@echo off

copy /y %THIS_DIR%obj\local\armeabi-v7a\libmbedtls.a  %THIS_DIR%libs\armeabi-v7a\
copy /y %THIS_DIR%obj\local\x86\libmbedtls.a          %THIS_DIR%libs\x86\

copy /y %THIS_DIR%obj\local\armeabi-v7a\libpro_util.a %THIS_DIR%libs\armeabi-v7a\
copy /y %THIS_DIR%obj\local\x86\libpro_util.a         %THIS_DIR%libs\x86\

@echo on
pause
