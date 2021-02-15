@echo off
set THIS_DIR=%~sdp0

copy /y %THIS_DIR%..\..\build\windows-vs6\_release32\*.exe                %THIS_DIR%windows-vs6\x86\
copy /y %THIS_DIR%..\..\build\windows-vs6\_release32\*.dll                %THIS_DIR%windows-vs6\x86\
copy /y %THIS_DIR%..\..\build\windows-vs6\_release32\*.lib                %THIS_DIR%windows-vs6\x86\
copy /y %THIS_DIR%..\..\build\windows-vs6\_release32\*.map                %THIS_DIR%windows-vs6\x86\
copy /y %THIS_DIR%..\..\build\windows-vs6\_release32\*.pdb                %THIS_DIR%windows-vs6\x86\

copy /y %THIS_DIR%..\..\build\windows-vs6\_release32-md\mbedtls_s-md.lib  %THIS_DIR%windows-vs6\x86\
copy /y %THIS_DIR%..\..\build\windows-vs6\_release32-md\mbedtls_s-md.pdb  %THIS_DIR%windows-vs6\x86\
copy /y %THIS_DIR%..\..\build\windows-vs6\_release32-md\pro_util_s-md.lib %THIS_DIR%windows-vs6\x86\
copy /y %THIS_DIR%..\..\build\windows-vs6\_release32-md\pro_util_s-md.pdb %THIS_DIR%windows-vs6\x86\

del %THIS_DIR%windows-vs6\x86\test_msg_client.map
del %THIS_DIR%windows-vs6\x86\test_msg_client.pdb
del %THIS_DIR%windows-vs6\x86\test_rtp.map
del %THIS_DIR%windows-vs6\x86\test_rtp.pdb
del %THIS_DIR%windows-vs6\x86\test_tcp_server.map
del %THIS_DIR%windows-vs6\x86\test_tcp_server.pdb
del %THIS_DIR%windows-vs6\x86\test_tcp_client.map
del %THIS_DIR%windows-vs6\x86\test_tcp_client.pdb

pause
