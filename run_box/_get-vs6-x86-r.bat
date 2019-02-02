@echo off
set THIS_DIR=%~sdp0

copy /y %THIS_DIR%..\pub\cfg\ca.crt                  %THIS_DIR%vs6-x86-r\
copy /y %THIS_DIR%..\pub\cfg\server.crt              %THIS_DIR%vs6-x86-r\
copy /y %THIS_DIR%..\pub\cfg\server.key              %THIS_DIR%vs6-x86-r\
copy /y %THIS_DIR%..\pub\cfg\*.cfg                   %THIS_DIR%vs6-x86-r\
copy /y %THIS_DIR%..\pub\cfg\rtp_msg_server.db       %THIS_DIR%vs6-x86-r\

copy /y %THIS_DIR%..\pub\lib-r\windows-vs6\x86\*.exe %THIS_DIR%vs6-x86-r\
copy /y %THIS_DIR%..\pub\lib-r\windows-vs6\x86\*.dll %THIS_DIR%vs6-x86-r\

pause
