@echo off
set THIS_DIR=%~sdp0

copy /y %THIS_DIR%..\..\build\windows-vs2010\_release\*.exe    %THIS_DIR%windows-vs2010\x86\
copy /y %THIS_DIR%..\..\build\windows-vs2010\_release\*.dll    %THIS_DIR%windows-vs2010\x86\
copy /y %THIS_DIR%..\..\build\windows-vs2010\_release\*.lib    %THIS_DIR%windows-vs2010\x86\
copy /y %THIS_DIR%..\..\build\windows-vs2010\_release\*.map    %THIS_DIR%windows-vs2010\x86\
copy /y %THIS_DIR%..\..\build\windows-vs2010\_release\*.pdb    %THIS_DIR%windows-vs2010\x86\

copy /y %THIS_DIR%..\..\build\windows-vs2010\_release64x\*.exe %THIS_DIR%windows-vs2010\x86_64\
copy /y %THIS_DIR%..\..\build\windows-vs2010\_release64x\*.dll %THIS_DIR%windows-vs2010\x86_64\
copy /y %THIS_DIR%..\..\build\windows-vs2010\_release64x\*.lib %THIS_DIR%windows-vs2010\x86_64\
copy /y %THIS_DIR%..\..\build\windows-vs2010\_release64x\*.map %THIS_DIR%windows-vs2010\x86_64\
copy /y %THIS_DIR%..\..\build\windows-vs2010\_release64x\*.pdb %THIS_DIR%windows-vs2010\x86_64\

pause
