@echo off

if exist .\libs\arm64-v8a      rmdir /s /q .\libs\arm64-v8a
if exist .\libs\x86_64         rmdir /s /q .\libs\x86_64
if exist .\libs\mips64         rmdir /s /q .\libs\mips64
if exist .\obj\local\arm64-v8a rmdir /s /q .\obj\local\arm64-v8a
if exist .\obj\local\x86_64    rmdir /s /q .\obj\local\x86_64
if exist .\obj\local\mips64    rmdir /s /q .\obj\local\mips64

@echo on
call ndk-build V=1 APP_ABI="arm64-v8a x86_64 mips64"
@echo off

copy /y .\obj\local\arm64-v8a\libmbedtls.a  .\libs\arm64-v8a\
copy /y .\obj\local\x86_64\libmbedtls.a     .\libs\x86_64\
copy /y .\obj\local\mips64\libmbedtls.a     .\libs\mips64\

copy /y .\obj\local\arm64-v8a\libpro_util.a .\libs\arm64-v8a\
copy /y .\obj\local\x86_64\libpro_util.a    .\libs\x86_64\
copy /y .\obj\local\mips64\libpro_util.a    .\libs\mips64\

@echo on
pause
