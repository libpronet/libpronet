@echo off
set THIS_DIR=%~sdp0

copy /y %THIS_DIR%..\pub\cfg\ca.crt                        %THIS_DIR%vs2013-x86_64-d\
copy /y %THIS_DIR%..\pub\cfg\server.crt                    %THIS_DIR%vs2013-x86_64-d\
copy /y %THIS_DIR%..\pub\cfg\server.key                    %THIS_DIR%vs2013-x86_64-d\
copy /y %THIS_DIR%..\pub\cfg\*.cfg                         %THIS_DIR%vs2013-x86_64-d\
copy /y %THIS_DIR%..\pub\cfg\rtp_msg_server.db             %THIS_DIR%vs2013-x86_64-d\

copy /y %THIS_DIR%..\pub\lib-d\windows-vs2013\x86_64\*.exe %THIS_DIR%vs2013-x86_64-d\
copy /y %THIS_DIR%..\pub\lib-d\windows-vs2013\x86_64\*.dll %THIS_DIR%vs2013-x86_64-d\

pause
