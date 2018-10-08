@echo off
set THIS_DIR=%~sdp0

copy /y %THIS_DIR%..\..\build\windows-vs2010\_debug\*.exe    %THIS_DIR%windows-vs2010\x86\
copy /y %THIS_DIR%..\..\build\windows-vs2010\_debug\*.dll    %THIS_DIR%windows-vs2010\x86\
copy /y %THIS_DIR%..\..\build\windows-vs2010\_debug\*.lib    %THIS_DIR%windows-vs2010\x86\
copy /y %THIS_DIR%..\..\build\windows-vs2010\_debug\*.map    %THIS_DIR%windows-vs2010\x86\
copy /y %THIS_DIR%..\..\build\windows-vs2010\_debug\*.pdb    %THIS_DIR%windows-vs2010\x86\

copy /y %THIS_DIR%..\..\build\windows-vs2010\_debug64x\*.exe %THIS_DIR%windows-vs2010\x86_64\
copy /y %THIS_DIR%..\..\build\windows-vs2010\_debug64x\*.dll %THIS_DIR%windows-vs2010\x86_64\
copy /y %THIS_DIR%..\..\build\windows-vs2010\_debug64x\*.lib %THIS_DIR%windows-vs2010\x86_64\
copy /y %THIS_DIR%..\..\build\windows-vs2010\_debug64x\*.map %THIS_DIR%windows-vs2010\x86_64\
copy /y %THIS_DIR%..\..\build\windows-vs2010\_debug64x\*.pdb %THIS_DIR%windows-vs2010\x86_64\

pause
