@echo off
set THIS_DIR=%~sdp0

copy /y %THIS_DIR%..\..\build\windows-vs2015\_debug32\*.exe                %THIS_DIR%windows-vs2015\x86\
copy /y %THIS_DIR%..\..\build\windows-vs2015\_debug32\*.dll                %THIS_DIR%windows-vs2015\x86\
copy /y %THIS_DIR%..\..\build\windows-vs2015\_debug32\*.lib                %THIS_DIR%windows-vs2015\x86\
copy /y %THIS_DIR%..\..\build\windows-vs2015\_debug32\*.map                %THIS_DIR%windows-vs2015\x86\
copy /y %THIS_DIR%..\..\build\windows-vs2015\_debug32\*.pdb                %THIS_DIR%windows-vs2015\x86\

copy /y %THIS_DIR%..\..\build\windows-vs2015\_debug32-md\mbedtls_s-md.lib  %THIS_DIR%windows-vs2015\x86\
copy /y %THIS_DIR%..\..\build\windows-vs2015\_debug32-md\mbedtls_s-md.pdb  %THIS_DIR%windows-vs2015\x86\
copy /y %THIS_DIR%..\..\build\windows-vs2015\_debug32-md\pro_util_s-md.lib %THIS_DIR%windows-vs2015\x86\
copy /y %THIS_DIR%..\..\build\windows-vs2015\_debug32-md\pro_util_s-md.pdb %THIS_DIR%windows-vs2015\x86\

copy /y %THIS_DIR%..\..\build\windows-vs2015\_debug64\*.exe                %THIS_DIR%windows-vs2015\x86_64\
copy /y %THIS_DIR%..\..\build\windows-vs2015\_debug64\*.dll                %THIS_DIR%windows-vs2015\x86_64\
copy /y %THIS_DIR%..\..\build\windows-vs2015\_debug64\*.lib                %THIS_DIR%windows-vs2015\x86_64\
copy /y %THIS_DIR%..\..\build\windows-vs2015\_debug64\*.map                %THIS_DIR%windows-vs2015\x86_64\
copy /y %THIS_DIR%..\..\build\windows-vs2015\_debug64\*.pdb                %THIS_DIR%windows-vs2015\x86_64\

copy /y %THIS_DIR%..\..\build\windows-vs2015\_debug64-md\mbedtls_s-md.lib  %THIS_DIR%windows-vs2015\x86_64\
copy /y %THIS_DIR%..\..\build\windows-vs2015\_debug64-md\mbedtls_s-md.pdb  %THIS_DIR%windows-vs2015\x86_64\
copy /y %THIS_DIR%..\..\build\windows-vs2015\_debug64-md\pro_util_s-md.lib %THIS_DIR%windows-vs2015\x86_64\
copy /y %THIS_DIR%..\..\build\windows-vs2015\_debug64-md\pro_util_s-md.pdb %THIS_DIR%windows-vs2015\x86_64\

del %THIS_DIR%windows-vs2015\x86\test_msg_client.map
del %THIS_DIR%windows-vs2015\x86\test_msg_client.pdb
del %THIS_DIR%windows-vs2015\x86\test_rtp.map
del %THIS_DIR%windows-vs2015\x86\test_rtp.pdb
del %THIS_DIR%windows-vs2015\x86\test_tcp_server.map
del %THIS_DIR%windows-vs2015\x86\test_tcp_server.pdb
del %THIS_DIR%windows-vs2015\x86\test_tcp_client.map
del %THIS_DIR%windows-vs2015\x86\test_tcp_client.pdb

del %THIS_DIR%windows-vs2015\x86_64\test_msg_client.map
del %THIS_DIR%windows-vs2015\x86_64\test_msg_client.pdb
del %THIS_DIR%windows-vs2015\x86_64\test_rtp.map
del %THIS_DIR%windows-vs2015\x86_64\test_rtp.pdb
del %THIS_DIR%windows-vs2015\x86_64\test_tcp_server.map
del %THIS_DIR%windows-vs2015\x86_64\test_tcp_server.pdb
del %THIS_DIR%windows-vs2015\x86_64\test_tcp_client.map
del %THIS_DIR%windows-vs2015\x86_64\test_tcp_client.pdb

pause
