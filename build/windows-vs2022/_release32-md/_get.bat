@echo off
set THIS_DIR=%~sdp0

copy /y %THIS_DIR%..\..\..\pub\cfg\ca.crt            %THIS_DIR%
copy /y %THIS_DIR%..\..\..\pub\cfg\server.crt        %THIS_DIR%
copy /y %THIS_DIR%..\..\..\pub\cfg\server.key        %THIS_DIR%
copy /y %THIS_DIR%..\..\..\pub\cfg\*.cfg             %THIS_DIR%
copy /y %THIS_DIR%..\..\..\pub\cfg\rtp_msg_server.db %THIS_DIR%

pause
