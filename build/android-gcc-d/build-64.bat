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
