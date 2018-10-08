# Microsoft Developer Studio Project File - Name="05_pro_rtp" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Dynamic-Link Library" 0x0102

CFG=05_pro_rtp - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "pro_rtp.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "pro_rtp.mak" CFG="05_pro_rtp - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "05_pro_rtp - Win32 Release" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE "05_pro_rtp - Win32 Debug" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe
MTL=midl.exe
RSC=rc.exe

!IF  "$(CFG)" == "05_pro_rtp - Win32 Release"

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
# ADD BASE CPP /nologo /MT /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "PRO_RTP_EXPORTS" /YX /FD /c
# ADD CPP /nologo /MT /W3 /GX /Zi /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D _WIN32_WINNT=0x0501 /D for="if (0) {} else for" /D "PRO_RTP_EXPORTS" /FR /FD /c
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
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /dll /map:"../_release/pro_rtp.map" /debug /machine:I386 /pdbtype:con /mapinfo:lines
# SUBTRACT LINK32 /pdb:none

!ELSEIF  "$(CFG)" == "05_pro_rtp - Win32 Debug"

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
# ADD BASE CPP /nologo /MTd /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "PRO_RTP_EXPORTS" /YX /FD /GZ /c
# ADD CPP /nologo /MTd /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D _WIN32_WINNT=0x0501 /D for="if (0) {} else for" /D "PRO_RTP_EXPORTS" /FR /FD /GZ /c
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
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /dll /map:"../_debug/pro_rtp.map" /debug /machine:I386 /pdbtype:con /mapinfo:lines
# SUBTRACT LINK32 /pdb:none

!ENDIF 

# Begin Target

# Name "05_pro_rtp - Win32 Release"
# Name "05_pro_rtp - Win32 Debug"
# Begin Group "rtp_framework"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\..\..\src\pro\pro_rtp\rtp_framework.cpp
# End Source File
# Begin Source File

SOURCE=..\..\..\src\pro\pro_rtp\rtp_framework.h
# End Source File
# Begin Source File

SOURCE=..\..\..\src\pro\pro_rtp\rtp_packet.cpp
# End Source File
# Begin Source File

SOURCE=..\..\..\src\pro\pro_rtp\rtp_packet.h
# End Source File
# Begin Source File

SOURCE=..\..\..\src\pro\pro_rtp\rtp_port_allocator.cpp
# End Source File
# Begin Source File

SOURCE=..\..\..\src\pro\pro_rtp\rtp_port_allocator.h
# End Source File
# Begin Source File

SOURCE=..\..\..\src\pro\pro_rtp\rtp_service.cpp
# End Source File
# Begin Source File

SOURCE=..\..\..\src\pro\pro_rtp\rtp_service.h
# End Source File
# Begin Source File

SOURCE=..\..\..\src\pro\pro_rtp\rtp_session_base.cpp
# End Source File
# Begin Source File

SOURCE=..\..\..\src\pro\pro_rtp\rtp_session_base.h
# End Source File
# Begin Source File

SOURCE=..\..\..\src\pro\pro_rtp\rtp_session_mcast.cpp
# End Source File
# Begin Source File

SOURCE=..\..\..\src\pro\pro_rtp\rtp_session_mcast.h
# End Source File
# Begin Source File

SOURCE=..\..\..\src\pro\pro_rtp\rtp_session_mcast_ex.cpp
# End Source File
# Begin Source File

SOURCE=..\..\..\src\pro\pro_rtp\rtp_session_mcast_ex.h
# End Source File
# Begin Source File

SOURCE=..\..\..\src\pro\pro_rtp\rtp_session_sslclient_ex.cpp
# End Source File
# Begin Source File

SOURCE=..\..\..\src\pro\pro_rtp\rtp_session_sslclient_ex.h
# End Source File
# Begin Source File

SOURCE=..\..\..\src\pro\pro_rtp\rtp_session_sslserver_ex.cpp
# End Source File
# Begin Source File

SOURCE=..\..\..\src\pro\pro_rtp\rtp_session_sslserver_ex.h
# End Source File
# Begin Source File

SOURCE=..\..\..\src\pro\pro_rtp\rtp_session_tcpclient.cpp
# End Source File
# Begin Source File

SOURCE=..\..\..\src\pro\pro_rtp\rtp_session_tcpclient.h
# End Source File
# Begin Source File

SOURCE=..\..\..\src\pro\pro_rtp\rtp_session_tcpclient_ex.cpp
# End Source File
# Begin Source File

SOURCE=..\..\..\src\pro\pro_rtp\rtp_session_tcpclient_ex.h
# End Source File
# Begin Source File

SOURCE=..\..\..\src\pro\pro_rtp\rtp_session_tcpserver.cpp
# End Source File
# Begin Source File

SOURCE=..\..\..\src\pro\pro_rtp\rtp_session_tcpserver.h
# End Source File
# Begin Source File

SOURCE=..\..\..\src\pro\pro_rtp\rtp_session_tcpserver_ex.cpp
# End Source File
# Begin Source File

SOURCE=..\..\..\src\pro\pro_rtp\rtp_session_tcpserver_ex.h
# End Source File
# Begin Source File

SOURCE=..\..\..\src\pro\pro_rtp\rtp_session_udpclient.cpp
# End Source File
# Begin Source File

SOURCE=..\..\..\src\pro\pro_rtp\rtp_session_udpclient.h
# End Source File
# Begin Source File

SOURCE=..\..\..\src\pro\pro_rtp\rtp_session_udpclient_ex.cpp
# End Source File
# Begin Source File

SOURCE=..\..\..\src\pro\pro_rtp\rtp_session_udpclient_ex.h
# End Source File
# Begin Source File

SOURCE=..\..\..\src\pro\pro_rtp\rtp_session_udpserver.cpp
# End Source File
# Begin Source File

SOURCE=..\..\..\src\pro\pro_rtp\rtp_session_udpserver.h
# End Source File
# Begin Source File

SOURCE=..\..\..\src\pro\pro_rtp\rtp_session_udpserver_ex.cpp
# End Source File
# Begin Source File

SOURCE=..\..\..\src\pro\pro_rtp\rtp_session_udpserver_ex.h
# End Source File
# End Group
# Begin Group "rtp_foundation"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\..\..\src\pro\pro_rtp\rtp_bucket.cpp
# End Source File
# Begin Source File

SOURCE=..\..\..\src\pro\pro_rtp\rtp_bucket.h
# End Source File
# Begin Source File

SOURCE=..\..\..\src\pro\pro_rtp\rtp_flow_stat.cpp
# End Source File
# Begin Source File

SOURCE=..\..\..\src\pro\pro_rtp\rtp_flow_stat.h
# End Source File
# Begin Source File

SOURCE=..\..\..\src\pro\pro_rtp\rtp_foundation.cpp
# End Source File
# Begin Source File

SOURCE=..\..\..\src\pro\pro_rtp\rtp_foundation.h
# End Source File
# Begin Source File

SOURCE=..\..\..\src\pro\pro_rtp\rtp_msg_c2s.cpp
# End Source File
# Begin Source File

SOURCE=..\..\..\src\pro\pro_rtp\rtp_msg_c2s.h
# End Source File
# Begin Source File

SOURCE=..\..\..\src\pro\pro_rtp\rtp_msg_client.cpp
# End Source File
# Begin Source File

SOURCE=..\..\..\src\pro\pro_rtp\rtp_msg_client.h
# End Source File
# Begin Source File

SOURCE=..\..\..\src\pro\pro_rtp\rtp_msg_command.h
# End Source File
# Begin Source File

SOURCE=..\..\..\src\pro\pro_rtp\rtp_msg_server.cpp
# End Source File
# Begin Source File

SOURCE=..\..\..\src\pro\pro_rtp\rtp_msg_server.h
# End Source File
# Begin Source File

SOURCE=..\..\..\src\pro\pro_rtp\rtp_session_wrapper.cpp
# End Source File
# Begin Source File

SOURCE=..\..\..\src\pro\pro_rtp\rtp_session_wrapper.h
# End Source File
# End Group
# Begin Group "Resource Files"

# PROP Default_Filter "ico;cur;bmp;dlg;rc2;rct;bin;rgs;gif;jpg;jpeg;jpe"
# Begin Source File

SOURCE=..\..\..\src\pro\pro_rtp\manifest.bin
# End Source File
# Begin Source File

SOURCE=..\..\..\src\pro\pro_rtp\pro_rtp.rc
# End Source File
# Begin Source File

SOURCE=..\..\..\src\pro\pro_rtp\resource.h
# End Source File
# End Group
# Begin Source File

SOURCE=..\..\..\src\pro\pro_rtp\pro_rtp.def
# End Source File
# End Target
# End Project
