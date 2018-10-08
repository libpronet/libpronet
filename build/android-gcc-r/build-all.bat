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
