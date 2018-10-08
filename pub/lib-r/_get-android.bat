@echo off
set THIS_DIR=%~sdp0

copy /y %THIS_DIR%..\..\build\android-gcc-r\libs\armeabi\*     %THIS_DIR%android-gcc\armeabi\
copy /y %THIS_DIR%..\..\build\android-gcc-r\libs\armeabi-v7a\* %THIS_DIR%android-gcc\armeabi-v7a\
copy /y %THIS_DIR%..\..\build\android-gcc-r\libs\x86\*         %THIS_DIR%android-gcc\x86\
copy /y %THIS_DIR%..\..\build\android-gcc-r\libs\mips\*        %THIS_DIR%android-gcc\mips\
copy /y %THIS_DIR%..\..\build\android-gcc-r\libs\arm64-v8a\*   %THIS_DIR%android-gcc\arm64-v8a\
copy /y %THIS_DIR%..\..\build\android-gcc-r\libs\x86_64\*      %THIS_DIR%android-gcc\x86_64\
copy /y %THIS_DIR%..\..\build\android-gcc-r\libs\mips64\*      %THIS_DIR%android-gcc\mips64\

pause
