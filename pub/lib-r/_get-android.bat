@echo off
set THIS_DIR=%~sdp0

copy /y %THIS_DIR%..\..\build\android-clang-r\libs\armeabi-v7a\* %THIS_DIR%android-clang\armeabi-v7a\
copy /y %THIS_DIR%..\..\build\android-clang-r\libs\x86\*         %THIS_DIR%android-clang\x86\
copy /y %THIS_DIR%..\..\build\android-clang-r\libs\arm64-v8a\*   %THIS_DIR%android-clang\arm64-v8a\
copy /y %THIS_DIR%..\..\build\android-clang-r\libs\x86_64\*      %THIS_DIR%android-clang\x86_64\

pause
