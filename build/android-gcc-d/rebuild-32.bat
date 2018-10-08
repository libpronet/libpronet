@echo off

if exist .\libs\armeabi          rmdir /s /q .\libs\armeabi
if exist .\libs\armeabi-v7a      rmdir /s /q .\libs\armeabi-v7a
if exist .\libs\x86              rmdir /s /q .\libs\x86
if exist .\libs\mips             rmdir /s /q .\libs\mips
if exist .\obj\local\armeabi     rmdir /s /q .\obj\local\armeabi
if exist .\obj\local\armeabi-v7a rmdir /s /q .\obj\local\armeabi-v7a
if exist .\obj\local\x86         rmdir /s /q .\obj\local\x86
if exist .\obj\local\mips        rmdir /s /q .\obj\local\mips

@echo on
call ndk-build V=1 APP_ABI="armeabi armeabi-v7a x86 mips"
@echo off

copy /y .\obj\local\armeabi\libmbedtls.a      .\libs\armeabi\
copy /y .\obj\local\armeabi-v7a\libmbedtls.a  .\libs\armeabi-v7a\
copy /y .\obj\local\x86\libmbedtls.a          .\libs\x86\
copy /y .\obj\local\mips\libmbedtls.a         .\libs\mips\

copy /y .\obj\local\armeabi\libpro_util.a     .\libs\armeabi\
copy /y .\obj\local\armeabi-v7a\libpro_util.a .\libs\armeabi-v7a\
copy /y .\obj\local\x86\libpro_util.a         .\libs\x86\
copy /y .\obj\local\mips\libpro_util.a        .\libs\mips\

@echo on
pause
