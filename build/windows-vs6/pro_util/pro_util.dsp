# Microsoft Developer Studio Project File - Name="03_pro_util" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Static Library" 0x0104

CFG=03_pro_util - Win32 Debug_MD
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "pro_util.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "pro_util.mak" CFG="03_pro_util - Win32 Debug_MD"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "03_pro_util - Win32 Release" (based on "Win32 (x86) Static Library")
!MESSAGE "03_pro_util - Win32 Debug" (based on "Win32 (x86) Static Library")
!MESSAGE "03_pro_util - Win32 Release_MD" (based on "Win32 (x86) Static Library")
!MESSAGE "03_pro_util - Win32 Debug_MD" (based on "Win32 (x86) Static Library")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe
RSC=rc.exe

!IF  "$(CFG)" == "03_pro_util - Win32 Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Release"
# PROP BASE Intermediate_Dir "Release"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "../_release32"
# PROP Intermediate_Dir "Release"
# PROP Target_Dir ""
MTL=midl.exe
LINK32=link.exe -lib
# ADD BASE CPP /nologo /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_MBCS" /D "_LIB" /YX /FD /c
# ADD CPP /nologo /MT /W3 /GX /Zi /O2 /I "../../../src/mbedtls/include" /I "../../../src/pronet/pro_shared" /I "../../../src/pronet/pro_util" /D "WIN32" /D "NDEBUG" /D "_MBCS" /D "_LIB" /D _WIN32_WINNT=0x0501 /D for="if (0) {} else for" /FR /Fd"../_release32/pro_util_s.pdb" /FD /c
# ADD BASE RSC /l 0x804 /d "NDEBUG"
# ADD RSC /l 0x804 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LIB32=link.exe -lib
# ADD BASE LIB32 /nologo
# ADD LIB32 /nologo /out:"../_release32/pro_util_s.lib"

!ELSEIF  "$(CFG)" == "03_pro_util - Win32 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Debug"
# PROP BASE Intermediate_Dir "Debug"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "../_debug32"
# PROP Intermediate_Dir "Debug"
# PROP Target_Dir ""
MTL=midl.exe
LINK32=link.exe -lib
# ADD BASE CPP /nologo /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_MBCS" /D "_LIB" /YX /FD /GZ /c
# ADD CPP /nologo /MTd /W3 /Gm /GX /ZI /Od /I "../../../src/mbedtls/include" /I "../../../src/pronet/pro_shared" /I "../../../src/pronet/pro_util" /D "WIN32" /D "_DEBUG" /D "_MBCS" /D "_LIB" /D _WIN32_WINNT=0x0501 /D for="if (0) {} else for" /FR /Fd"../_debug32/pro_util_s.pdb" /FD /GZ /c
# ADD BASE RSC /l 0x804 /d "_DEBUG"
# ADD RSC /l 0x804 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LIB32=link.exe -lib
# ADD BASE LIB32 /nologo
# ADD LIB32 /nologo /out:"../_debug32/pro_util_s.lib"

!ELSEIF  "$(CFG)" == "03_pro_util - Win32 Release_MD"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "03_pro_util___Win32_Release_MD"
# PROP BASE Intermediate_Dir "03_pro_util___Win32_Release_MD"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "../_release32-md"
# PROP Intermediate_Dir "Release_MD"
# PROP Target_Dir ""
MTL=midl.exe
LINK32=link.exe -lib
# ADD BASE CPP /nologo /MT /W3 /GX /Zi /O2 /I "../../../src/mbedtls/include" /D "WIN32" /D "NDEBUG" /D "_MBCS" /D "_LIB" /D _WIN32_WINNT=0x0501 /D for="if (0) {} else for" /FR /Fd"../_release/pro_util_s.pdb" /FD /c
# ADD CPP /nologo /MD /W3 /GX /Zi /O2 /I "../../../src/mbedtls/include" /I "../../../src/pronet/pro_shared" /I "../../../src/pronet/pro_util" /D "WIN32" /D "NDEBUG" /D "_MBCS" /D "_LIB" /D _WIN32_WINNT=0x0501 /D for="if (0) {} else for" /FR /Fd"../_release32-md/pro_util_s-md.pdb" /FD /c
# ADD BASE RSC /l 0x804 /d "NDEBUG"
# ADD RSC /l 0x804 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LIB32=link.exe -lib
# ADD BASE LIB32 /nologo /out:"../_release/pro_util_s-mt.lib"
# ADD LIB32 /nologo /out:"../_release32-md/pro_util_s-md.lib"

!ELSEIF  "$(CFG)" == "03_pro_util - Win32 Debug_MD"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "03_pro_util___Win32_Debug_MD"
# PROP BASE Intermediate_Dir "03_pro_util___Win32_Debug_MD"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "../_debug32-md"
# PROP Intermediate_Dir "Debug_MD"
# PROP Target_Dir ""
MTL=midl.exe
LINK32=link.exe -lib
# ADD BASE CPP /nologo /MTd /W3 /Gm /GX /ZI /Od /I "../../../src/mbedtls/include" /D "WIN32" /D "_DEBUG" /D "_MBCS" /D "_LIB" /D _WIN32_WINNT=0x0501 /D for="if (0) {} else for" /FR /Fd"../_debug/pro_util_s.pdb" /FD /GZ /c
# ADD CPP /nologo /MDd /W3 /Gm /GX /ZI /Od /I "../../../src/mbedtls/include" /I "../../../src/pronet/pro_shared" /I "../../../src/pronet/pro_util" /D "WIN32" /D "_DEBUG" /D "_MBCS" /D "_LIB" /D _WIN32_WINNT=0x0501 /D for="if (0) {} else for" /FR /Fd"../_debug32-md/pro_util_s-md.pdb" /FD /GZ /c
# ADD BASE RSC /l 0x804 /d "_DEBUG"
# ADD RSC /l 0x804 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LIB32=link.exe -lib
# ADD BASE LIB32 /nologo /out:"../_debug/pro_util_s-mt.lib"
# ADD LIB32 /nologo /out:"../_debug32-md/pro_util_s-md.lib"

!ENDIF 

# Begin Target

# Name "03_pro_util - Win32 Release"
# Name "03_pro_util - Win32 Debug"
# Name "03_pro_util - Win32 Release_MD"
# Name "03_pro_util - Win32 Debug_MD"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Source File

SOURCE=..\..\..\src\pronet\pro_util\pro_bsd_wrapper.cpp
# End Source File
# Begin Source File

SOURCE=..\..\..\src\pronet\pro_util\pro_buffer.cpp
# End Source File
# Begin Source File

SOURCE=..\..\..\src\pronet\pro_util\pro_channel_task_pool.cpp
# End Source File
# Begin Source File

SOURCE=..\..\..\src\pronet\pro_util\pro_config_file.cpp
# End Source File
# Begin Source File

SOURCE=..\..\..\src\pronet\pro_util\pro_config_stream.cpp
# End Source File
# Begin Source File

SOURCE=..\..\..\src\pronet\pro_util\pro_file_monitor.cpp
# End Source File
# Begin Source File

SOURCE=..\..\..\src\pronet\pro_util\pro_functor_command_task.cpp
# End Source File
# Begin Source File

SOURCE=..\..\..\src\pronet\pro_util\pro_log_file.cpp
# End Source File
# Begin Source File

SOURCE=..\..\..\src\pronet\pro_util\pro_memory_pool.cpp
# End Source File
# Begin Source File

SOURCE=..\..\..\src\pronet\pro_util\pro_ref_count.cpp
# End Source File
# Begin Source File

SOURCE=..\..\..\src\pronet\pro_util\pro_shaper.cpp
# End Source File
# Begin Source File

SOURCE=..\..\..\src\pronet\pro_util\pro_ssl_util.cpp
# End Source File
# Begin Source File

SOURCE=..\..\..\src\pronet\pro_util\pro_stat.cpp
# End Source File
# Begin Source File

SOURCE=..\..\..\src\pronet\pro_util\pro_thread.cpp
# End Source File
# Begin Source File

SOURCE=..\..\..\src\pronet\pro_util\pro_thread_mutex.cpp
# End Source File
# Begin Source File

SOURCE=..\..\..\src\pronet\pro_util\pro_time_util.cpp
# End Source File
# Begin Source File

SOURCE=..\..\..\src\pronet\pro_util\pro_timer_factory.cpp
# End Source File
# Begin Source File

SOURCE=..\..\..\src\pronet\pro_util\pro_unicode.cpp
# End Source File
# Begin Source File

SOURCE=..\..\..\src\pronet\pro_util\pro_z.cpp
# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl"
# Begin Source File

SOURCE=..\..\..\src\pronet\pro_util\pro_a.h
# End Source File
# Begin Source File

SOURCE=..\..\..\src\pronet\pro_util\pro_bsd_wrapper.h
# End Source File
# Begin Source File

SOURCE=..\..\..\src\pronet\pro_util\pro_buffer.h
# End Source File
# Begin Source File

SOURCE=..\..\..\src\pronet\pro_util\pro_channel_task_pool.h
# End Source File
# Begin Source File

SOURCE=..\..\..\src\pronet\pro_util\pro_config_file.h
# End Source File
# Begin Source File

SOURCE=..\..\..\src\pronet\pro_util\pro_config_stream.h
# End Source File
# Begin Source File

SOURCE=..\..\..\src\pronet\pro_util\pro_file_monitor.h
# End Source File
# Begin Source File

SOURCE=..\..\..\src\pronet\pro_util\pro_functor_command.h
# End Source File
# Begin Source File

SOURCE=..\..\..\src\pronet\pro_util\pro_functor_command_task.h
# End Source File
# Begin Source File

SOURCE=..\..\..\src\pronet\pro_util\pro_log_file.h
# End Source File
# Begin Source File

SOURCE=..\..\..\src\pronet\pro_util\pro_memory_pool.h
# End Source File
# Begin Source File

SOURCE=..\..\..\src\pronet\pro_util\pro_ref_count.h
# End Source File
# Begin Source File

SOURCE=..\..\..\src\pronet\pro_util\pro_shaper.h
# End Source File
# Begin Source File

SOURCE=..\..\..\src\pronet\pro_util\pro_ssl_util.h
# End Source File
# Begin Source File

SOURCE=..\..\..\src\pronet\pro_util\pro_stat.h
# End Source File
# Begin Source File

SOURCE=..\..\..\src\pronet\pro_util\pro_stl.h
# End Source File
# Begin Source File

SOURCE=..\..\..\src\pronet\pro_util\pro_thread.h
# End Source File
# Begin Source File

SOURCE=..\..\..\src\pronet\pro_util\pro_thread_mutex.h
# End Source File
# Begin Source File

SOURCE=..\..\..\src\pronet\pro_util\pro_time_util.h
# End Source File
# Begin Source File

SOURCE=..\..\..\src\pronet\pro_util\pro_timer_factory.h
# End Source File
# Begin Source File

SOURCE=..\..\..\src\pronet\pro_util\pro_unicode.h
# End Source File
# Begin Source File

SOURCE=..\..\..\src\pronet\pro_util\pro_version.h
# End Source File
# Begin Source File

SOURCE=..\..\..\src\pronet\pro_util\pro_z.h
# End Source File
# End Group
# End Target
# End Project
