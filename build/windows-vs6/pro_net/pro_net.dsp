# Microsoft Developer Studio Project File - Name="04_pro_net" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Dynamic-Link Library" 0x0102

CFG=04_pro_net - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "pro_net.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "pro_net.mak" CFG="04_pro_net - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "04_pro_net - Win32 Release" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE "04_pro_net - Win32 Debug" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe
MTL=midl.exe
RSC=rc.exe

!IF  "$(CFG)" == "04_pro_net - Win32 Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Release"
# PROP BASE Intermediate_Dir "Release"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "../_release"
# PROP Intermediate_Dir "Release"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MT /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "PRO_NET_EXPORTS" /YX /FD /c
# ADD CPP /nologo /MT /W3 /GX /Zi /O2 /I "../../../src/mbedtls/include" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D _WIN32_WINNT=0x0501 /D for="if (0) {} else for" /D "PRO_NET_EXPORTS" /FR /FD /c
# SUBTRACT CPP /YX
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x804 /d "NDEBUG"
# ADD RSC /l 0x804 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /dll /machine:I386
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /dll /map:"../_release/pro_net.map" /debug /machine:I386 /pdbtype:con /mapinfo:lines
# SUBTRACT LINK32 /pdb:none

!ELSEIF  "$(CFG)" == "04_pro_net - Win32 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Debug"
# PROP BASE Intermediate_Dir "Debug"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "../_debug"
# PROP Intermediate_Dir "Debug"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MTd /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "PRO_NET_EXPORTS" /YX /FD /GZ /c
# ADD CPP /nologo /MTd /W3 /Gm /GX /ZI /Od /I "../../../src/mbedtls/include" /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D _WIN32_WINNT=0x0501 /D for="if (0) {} else for" /D "PRO_NET_EXPORTS" /FR /FD /GZ /c
# SUBTRACT CPP /YX
# ADD BASE MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x804 /d "_DEBUG"
# ADD RSC /l 0x804 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /dll /debug /machine:I386 /pdbtype:sept
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /dll /map:"../_debug/pro_net.map" /debug /machine:I386 /pdbtype:con /mapinfo:lines
# SUBTRACT LINK32 /pdb:none

!ENDIF 

# Begin Target

# Name "04_pro_net - Win32 Release"
# Name "04_pro_net - Win32 Debug"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Source File

SOURCE=..\..\..\src\pronet\pro_net\pro_acceptor.cpp
# End Source File
# Begin Source File

SOURCE=..\..\..\src\pronet\pro_net\pro_base_reactor.cpp
# End Source File
# Begin Source File

SOURCE=..\..\..\src\pronet\pro_net\pro_connector.cpp
# End Source File
# Begin Source File

SOURCE=..\..\..\src\pronet\pro_net\pro_epoll_reactor.cpp
# End Source File
# Begin Source File

SOURCE=..\..\..\src\pronet\pro_net\pro_handler_mgr.cpp
# End Source File
# Begin Source File

SOURCE=..\..\..\src\pronet\pro_net\pro_mbedtls.cpp
# End Source File
# Begin Source File

SOURCE=..\..\..\src\pronet\pro_net\pro_mcast_transport.cpp
# End Source File
# Begin Source File

SOURCE=..\..\..\src\pronet\pro_net\pro_net.cpp
# End Source File
# Begin Source File

SOURCE=..\..\..\src\pronet\pro_net\pro_net.def
# End Source File
# Begin Source File

SOURCE=..\..\..\src\pronet\pro_net\pro_notify_pipe.cpp
# End Source File
# Begin Source File

SOURCE=..\..\..\src\pronet\pro_net\pro_select_reactor.cpp
# End Source File
# Begin Source File

SOURCE=..\..\..\src\pronet\pro_net\pro_service_host.cpp
# End Source File
# Begin Source File

SOURCE=..\..\..\src\pronet\pro_net\pro_service_hub.cpp
# End Source File
# Begin Source File

SOURCE=..\..\..\src\pronet\pro_net\pro_service_pipe.cpp
# End Source File
# Begin Source File

SOURCE=..\..\..\src\pronet\pro_net\pro_ssl_handshaker.cpp
# End Source File
# Begin Source File

SOURCE=..\..\..\src\pronet\pro_net\pro_ssl_transport.cpp
# End Source File
# Begin Source File

SOURCE=..\..\..\src\pronet\pro_net\pro_tcp_handshaker.cpp
# End Source File
# Begin Source File

SOURCE=..\..\..\src\pronet\pro_net\pro_tcp_transport.cpp
# End Source File
# Begin Source File

SOURCE=..\..\..\src\pronet\pro_net\pro_tp_reactor_task.cpp
# End Source File
# Begin Source File

SOURCE=..\..\..\src\pronet\pro_net\pro_udp_transport.cpp
# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl"
# Begin Source File

SOURCE=..\..\..\src\pronet\pro_net\pro_acceptor.h
# End Source File
# Begin Source File

SOURCE=..\..\..\src\pronet\pro_net\pro_base_reactor.h
# End Source File
# Begin Source File

SOURCE=..\..\..\src\pronet\pro_net\pro_connector.h
# End Source File
# Begin Source File

SOURCE=..\..\..\src\pronet\pro_net\pro_epoll_reactor.h
# End Source File
# Begin Source File

SOURCE=..\..\..\src\pronet\pro_net\pro_event_handler.h
# End Source File
# Begin Source File

SOURCE=..\..\..\src\pronet\pro_net\pro_handler_mgr.h
# End Source File
# Begin Source File

SOURCE=..\..\..\src\pronet\pro_net\pro_mbedtls.h
# End Source File
# Begin Source File

SOURCE=..\..\..\src\pronet\pro_net\pro_mcast_transport.h
# End Source File
# Begin Source File

SOURCE=..\..\..\src\pronet\pro_net\pro_net.h
# End Source File
# Begin Source File

SOURCE=..\..\..\src\pronet\pro_net\pro_notify_pipe.h
# End Source File
# Begin Source File

SOURCE=..\..\..\src\pronet\pro_net\pro_recv_pool.h
# End Source File
# Begin Source File

SOURCE=..\..\..\src\pronet\pro_net\pro_select_reactor.h
# End Source File
# Begin Source File

SOURCE=..\..\..\src\pronet\pro_net\pro_send_pool.h
# End Source File
# Begin Source File

SOURCE=..\..\..\src\pronet\pro_net\pro_service_host.h
# End Source File
# Begin Source File

SOURCE=..\..\..\src\pronet\pro_net\pro_service_hub.h
# End Source File
# Begin Source File

SOURCE=..\..\..\src\pronet\pro_net\pro_service_pipe.h
# End Source File
# Begin Source File

SOURCE=..\..\..\src\pronet\pro_net\pro_ssl_handshaker.h
# End Source File
# Begin Source File

SOURCE=..\..\..\src\pronet\pro_net\pro_ssl_transport.h
# End Source File
# Begin Source File

SOURCE=..\..\..\src\pronet\pro_net\pro_tcp_handshaker.h
# End Source File
# Begin Source File

SOURCE=..\..\..\src\pronet\pro_net\pro_tcp_transport.h
# End Source File
# Begin Source File

SOURCE=..\..\..\src\pronet\pro_net\pro_tp_reactor_task.h
# End Source File
# Begin Source File

SOURCE=..\..\..\src\pronet\pro_net\pro_udp_transport.h
# End Source File
# End Group
# Begin Group "Resource Files"

# PROP Default_Filter "ico;cur;bmp;dlg;rc2;rct;bin;rgs;gif;jpg;jpeg;jpe"
# Begin Source File

SOURCE=..\..\..\src\pronet\pro_net\manifest.bin
# End Source File
# Begin Source File

SOURCE=..\..\..\src\pronet\pro_net\pro_net.rc
# End Source File
# Begin Source File

SOURCE=..\..\..\src\pronet\pro_net\resource.h
# End Source File
# End Group
# End Target
# End Project
