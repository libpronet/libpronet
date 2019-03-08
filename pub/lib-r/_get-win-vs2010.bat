@echo off
set THIS_DIR=%~sdp0

copy /y %THIS_DIR%..\..\build\windows-vs2010\_release\*.exe                   %THIS_DIR%windows-vs2010\x86\
copy /y %THIS_DIR%..\..\build\windows-vs2010\_release\*.dll                   %THIS_DIR%windows-vs2010\x86\
copy /y %THIS_DIR%..\..\build\windows-vs2010\_release\*.lib                   %THIS_DIR%windows-vs2010\x86\
copy /y %THIS_DIR%..\..\build\windows-vs2010\_release\*.map                   %THIS_DIR%windows-vs2010\x86\
copy /y %THIS_DIR%..\..\build\windows-vs2010\_release\*.pdb                   %THIS_DIR%windows-vs2010\x86\

copy /y %THIS_DIR%..\..\build\windows-vs2010\_release-md\mbedtls_s-md.lib     %THIS_DIR%windows-vs2010\x86\
copy /y %THIS_DIR%..\..\build\windows-vs2010\_release-md\mbedtls_s-md.pdb     %THIS_DIR%windows-vs2010\x86\
copy /y %THIS_DIR%..\..\build\windows-vs2010\_release-md\pro_util_s-md.lib    %THIS_DIR%windows-vs2010\x86\
copy /y %THIS_DIR%..\..\build\windows-vs2010\_release-md\pro_util_s-md.pdb    %THIS_DIR%windows-vs2010\x86\

copy /y %THIS_DIR%..\..\build\windows-vs2010\_release64x\*.exe                %THIS_DIR%windows-vs2010\x86_64\
copy /y %THIS_DIR%..\..\build\windows-vs2010\_release64x\*.dll                %THIS_DIR%windows-vs2010\x86_64\
copy /y %THIS_DIR%..\..\build\windows-vs2010\_release64x\*.lib                %THIS_DIR%windows-vs2010\x86_64\
copy /y %THIS_DIR%..\..\build\windows-vs2010\_release64x\*.map                %THIS_DIR%windows-vs2010\x86_64\
copy /y %THIS_DIR%..\..\build\windows-vs2010\_release64x\*.pdb                %THIS_DIR%windows-vs2010\x86_64\

copy /y %THIS_DIR%..\..\build\windows-vs2010\_release64x-md\mbedtls_s-md.lib  %THIS_DIR%windows-vs2010\x86_64\
copy /y %THIS_DIR%..\..\build\windows-vs2010\_release64x-md\mbedtls_s-md.pdb  %THIS_DIR%windows-vs2010\x86_64\
copy /y %THIS_DIR%..\..\build\windows-vs2010\_release64x-md\pro_util_s-md.lib %THIS_DIR%windows-vs2010\x86_64\
copy /y %THIS_DIR%..\..\build\windows-vs2010\_release64x-md\pro_util_s-md.pdb %THIS_DIR%windows-vs2010\x86_64\

del %THIS_DIR%windows-vs2010\x86\test_msg_client.map
del %THIS_DIR%windows-vs2010\x86\test_msg_client.pdb
del %THIS_DIR%windows-vs2010\x86\test_rtp.map
del %THIS_DIR%windows-vs2010\x86\test_rtp.pdb
del %THIS_DIR%windows-vs2010\x86\test_tcp_server.map
del %THIS_DIR%windows-vs2010\x86\test_tcp_server.pdb
del %THIS_DIR%windows-vs2010\x86\test_tcp_client.map
del %THIS_DIR%windows-vs2010\x86\test_tcp_client.pdb

del %THIS_DIR%windows-vs2010\x86_64\test_msg_client.map
del %THIS_DIR%windows-vs2010\x86_64\test_msg_client.pdb
del %THIS_DIR%windows-vs2010\x86_64\test_rtp.map
del %THIS_DIR%windows-vs2010\x86_64\test_rtp.pdb
del %THIS_DIR%windows-vs2010\x86_64\test_tcp_server.map
del %THIS_DIR%windows-vs2010\x86_64\test_tcp_server.pdb
del %THIS_DIR%windows-vs2010\x86_64\test_tcp_client.map
del %THIS_DIR%windows-vs2010\x86_64\test_tcp_client.pdb

pause
