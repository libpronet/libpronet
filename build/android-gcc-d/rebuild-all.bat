@echo off

if exist .\libs\armeabi          rmdir /s /q .\libs\armeabi
if exist .\libs\armeabi-v7a      rmdir /s /q .\libs\armeabi-v7a
if exist .\libs\x86              rmdir /s /q .\libs\x86
if exist .\libs\mips             rmdir /s /q .\libs\mips
if exist .\libs\arm64-v8a        rmdir /s /q .\libs\arm64-v8a
if exist .\libs\x86_64           rmdir /s /q .\libs\x86_64
if exist .\libs\mips64           rmdir /s /q .\libs\mips64
if exist .\obj\local\armeabi     rmdir /s /q .\obj\local\armeabi
if exist .\obj\local\armeabi-v7a rmdir /s /q .\obj\local\armeabi-v7a
if exist .\obj\local\x86         rmdir /s /q .\obj\local\x86
if exist .\obj\local\mips        rmdir /s /q .\obj\local\mips
if exist .\obj\local\arm64-v8a   rmdir /s /q .\obj\local\arm64-v8a
if exist .\obj\local\x86_64      rmdir /s /q .\obj\local\x86_64
if exist .\obj\local\mips64      rmdir /s /q .\obj\local\mips64

@echo on
call ndk-build V=1 APP_ABI="armeabi armeabi-v7a x86 mips arm64-v8a x86_64 mips64"
@echo off

copy /y .\obj\local\armeabi\libmbedtls.a      .\libs\armeabi\
copy /y .\obj\local\armeabi-v7a\libmbedtls.a  .\libs\armeabi-v7a\
copy /y .\obj\local\x86\libmbedtls.a          .\libs\x86\
copy /y .\obj\local\mips\libmbedtls.a         .\libs\mips\
copy /y .\obj\local\arm64-v8a\libmbedtls.a    .\libs\arm64-v8a\
copy /y .\obj\local\x86_64\libmbedtls.a       .\libs\x86_64\
copy /y .\obj\local\mips64\libmbedtls.a       .\libs\mips64\

copy /y .\obj\local\armeabi\libpro_util.a     .\libs\armeabi\
copy /y .\obj\local\armeabi-v7a\libpro_util.a .\libs\armeabi-v7a\
copy /y .\obj\local\x86\libpro_util.a         .\libs\x86\
copy /y .\obj\local\mips\libpro_util.a        .\libs\mips\
copy /y .\obj\local\arm64-v8a\libpro_util.a   .\libs\arm64-v8a\
copy /y .\obj\local\x86_64\libpro_util.a      .\libs\x86_64\
copy /y .\obj\local\mips64\libpro_util.a      .\libs\mips64\

@echo on
pause
