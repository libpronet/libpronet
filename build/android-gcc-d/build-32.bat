call ndk-build V=1 APP_ABI="armeabi armeabi-v7a x86 mips"

@echo off
set THIS_DIR=%~sdp0

copy /y %THIS_DIR%obj\local\armeabi\libmbedtls.a      %THIS_DIR%libs\armeabi\
copy /y %THIS_DIR%obj\local\armeabi-v7a\libmbedtls.a  %THIS_DIR%libs\armeabi-v7a\
copy /y %THIS_DIR%obj\local\x86\libmbedtls.a          %THIS_DIR%libs\x86\
copy /y %THIS_DIR%obj\local\mips\libmbedtls.a         %THIS_DIR%libs\mips\

copy /y %THIS_DIR%obj\local\armeabi\libpro_util.a     %THIS_DIR%libs\armeabi\
copy /y %THIS_DIR%obj\local\armeabi-v7a\libpro_util.a %THIS_DIR%libs\armeabi-v7a\
copy /y %THIS_DIR%obj\local\x86\libpro_util.a         %THIS_DIR%libs\x86\
copy /y %THIS_DIR%obj\local\mips\libpro_util.a        %THIS_DIR%libs\mips\

@echo on
pause
