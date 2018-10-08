# Microsoft Developer Studio Project File - Name="01_mbedtls" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Static Library" 0x0104

CFG=01_mbedtls - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "mbedtls.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "mbedtls.mak" CFG="01_mbedtls - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "01_mbedtls - Win32 Release" (based on "Win32 (x86) Static Library")
!MESSAGE "01_mbedtls - Win32 Debug" (based on "Win32 (x86) Static Library")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe
RSC=rc.exe

!IF  "$(CFG)" == "01_mbedtls - Win32 Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Release"
# PROP BASE Intermediate_Dir "Release"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "../_release"
# PROP Intermediate_Dir "Release"
# PROP Target_Dir ""
MTL=midl.exe
LINK32=link.exe -lib
# ADD BASE CPP /nologo /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_MBCS" /D "_LIB" /YX /FD /c
# ADD CPP /nologo /MT /W3 /GX /Zi /O2 /I "../../../src/mbedtls/include" /D "WIN32" /D "NDEBUG" /D "_MBCS" /D "_LIB" /D _WIN32_WINNT=0x0501 /D for="if (0) {} else for" /FR /Fd"../_release/mbedtls_s.pdb" /FD /c
# ADD BASE RSC /l 0x804 /d "NDEBUG"
# ADD RSC /l 0x804 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LIB32=link.exe -lib
# ADD BASE LIB32 /nologo
# ADD LIB32 /nologo /out:"../_release/mbedtls_s.lib"

!ELSEIF  "$(CFG)" == "01_mbedtls - Win32 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Debug"
# PROP BASE Intermediate_Dir "Debug"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "../_debug"
# PROP Intermediate_Dir "Debug"
# PROP Target_Dir ""
MTL=midl.exe
LINK32=link.exe -lib
# ADD BASE CPP /nologo /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_MBCS" /D "_LIB" /YX /FD /GZ /c
# ADD CPP /nologo /MTd /W3 /Gm /GX /ZI /Od /I "../../../src/mbedtls/include" /D "WIN32" /D "_DEBUG" /D "_MBCS" /D "_LIB" /D _WIN32_WINNT=0x0501 /D for="if (0) {} else for" /FR /Fd"../_debug/mbedtls_s.pdb" /FD /GZ /c
# ADD BASE RSC /l 0x804 /d "_DEBUG"
# ADD RSC /l 0x804 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LIB32=link.exe -lib
# ADD BASE LIB32 /nologo
# ADD LIB32 /nologo /out:"../_debug/mbedtls_s.lib"

!ENDIF 

# Begin Target

# Name "01_mbedtls - Win32 Release"
# Name "01_mbedtls - Win32 Debug"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Source File

SOURCE=..\..\..\src\mbedtls\library\aes.c
# End Source File
# Begin Source File

SOURCE=..\..\..\src\mbedtls\library\aesni.c
# End Source File
# Begin Source File

SOURCE=..\..\..\src\mbedtls\library\arc4.c
# End Source File
# Begin Source File

SOURCE=..\..\..\src\mbedtls\library\aria.c
# End Source File
# Begin Source File

SOURCE=..\..\..\src\mbedtls\library\asn1parse.c
# End Source File
# Begin Source File

SOURCE=..\..\..\src\mbedtls\library\asn1write.c
# End Source File
# Begin Source File

SOURCE=..\..\..\src\mbedtls\library\base64.c
# End Source File
# Begin Source File

SOURCE=..\..\..\src\mbedtls\library\bignum.c
# End Source File
# Begin Source File

SOURCE=..\..\..\src\mbedtls\library\blowfish.c
# End Source File
# Begin Source File

SOURCE=..\..\..\src\mbedtls\library\camellia.c
# End Source File
# Begin Source File

SOURCE=..\..\..\src\mbedtls\library\ccm.c
# End Source File
# Begin Source File

SOURCE=..\..\..\src\mbedtls\library\certs.c
# End Source File
# Begin Source File

SOURCE=..\..\..\src\mbedtls\library\chacha20.c
# End Source File
# Begin Source File

SOURCE=..\..\..\src\mbedtls\library\chachapoly.c
# End Source File
# Begin Source File

SOURCE=..\..\..\src\mbedtls\library\cipher.c
# End Source File
# Begin Source File

SOURCE=..\..\..\src\mbedtls\library\cipher_wrap.c
# End Source File
# Begin Source File

SOURCE=..\..\..\src\mbedtls\library\cmac.c
# End Source File
# Begin Source File

SOURCE=..\..\..\src\mbedtls\library\ctr_drbg.c
# End Source File
# Begin Source File

SOURCE=..\..\..\src\mbedtls\library\debug.c
# End Source File
# Begin Source File

SOURCE=..\..\..\src\mbedtls\library\des.c
# End Source File
# Begin Source File

SOURCE=..\..\..\src\mbedtls\library\dhm.c
# End Source File
# Begin Source File

SOURCE=..\..\..\src\mbedtls\library\ecdh.c
# End Source File
# Begin Source File

SOURCE=..\..\..\src\mbedtls\library\ecdsa.c
# End Source File
# Begin Source File

SOURCE=..\..\..\src\mbedtls\library\ecjpake.c
# End Source File
# Begin Source File

SOURCE=..\..\..\src\mbedtls\library\ecp.c
# End Source File
# Begin Source File

SOURCE=..\..\..\src\mbedtls\library\ecp_curves.c
# End Source File
# Begin Source File

SOURCE=..\..\..\src\mbedtls\library\entropy.c
# End Source File
# Begin Source File

SOURCE=..\..\..\src\mbedtls\library\entropy_poll.c
# End Source File
# Begin Source File

SOURCE=..\..\..\src\mbedtls\library\error.c
# End Source File
# Begin Source File

SOURCE=..\..\..\src\mbedtls\library\gcm.c
# End Source File
# Begin Source File

SOURCE=..\..\..\src\mbedtls\library\havege.c
# End Source File
# Begin Source File

SOURCE=..\..\..\src\mbedtls\library\hkdf.c
# End Source File
# Begin Source File

SOURCE=..\..\..\src\mbedtls\library\hmac_drbg.c
# End Source File
# Begin Source File

SOURCE=..\..\..\src\mbedtls\library\md.c
# End Source File
# Begin Source File

SOURCE=..\..\..\src\mbedtls\library\md2.c
# End Source File
# Begin Source File

SOURCE=..\..\..\src\mbedtls\library\md4.c
# End Source File
# Begin Source File

SOURCE=..\..\..\src\mbedtls\library\md5.c
# End Source File
# Begin Source File

SOURCE=..\..\..\src\mbedtls\library\md_wrap.c
# End Source File
# Begin Source File

SOURCE=..\..\..\src\mbedtls\library\memory_buffer_alloc.c
# End Source File
# Begin Source File

SOURCE=..\..\..\src\mbedtls\library\net_sockets.c
# End Source File
# Begin Source File

SOURCE=..\..\..\src\mbedtls\library\nist_kw.c
# End Source File
# Begin Source File

SOURCE=..\..\..\src\mbedtls\library\oid.c
# End Source File
# Begin Source File

SOURCE=..\..\..\src\mbedtls\library\padlock.c
# End Source File
# Begin Source File

SOURCE=..\..\..\src\mbedtls\library\pem.c
# End Source File
# Begin Source File

SOURCE=..\..\..\src\mbedtls\library\pk.c
# End Source File
# Begin Source File

SOURCE=..\..\..\src\mbedtls\library\pk_wrap.c
# End Source File
# Begin Source File

SOURCE=..\..\..\src\mbedtls\library\pkcs11.c
# End Source File
# Begin Source File

SOURCE=..\..\..\src\mbedtls\library\pkcs12.c
# End Source File
# Begin Source File

SOURCE=..\..\..\src\mbedtls\library\pkcs5.c
# End Source File
# Begin Source File

SOURCE=..\..\..\src\mbedtls\library\pkparse.c
# End Source File
# Begin Source File

SOURCE=..\..\..\src\mbedtls\library\pkwrite.c
# End Source File
# Begin Source File

SOURCE=..\..\..\src\mbedtls\library\platform.c
# End Source File
# Begin Source File

SOURCE=..\..\..\src\mbedtls\library\platform_util.c
# End Source File
# Begin Source File

SOURCE=..\..\..\src\mbedtls\library\poly1305.c
# End Source File
# Begin Source File

SOURCE=..\..\..\src\mbedtls\library\ripemd160.c
# End Source File
# Begin Source File

SOURCE=..\..\..\src\mbedtls\library\rsa.c
# End Source File
# Begin Source File

SOURCE=..\..\..\src\mbedtls\library\rsa_internal.c
# End Source File
# Begin Source File

SOURCE=..\..\..\src\mbedtls\library\sha1.c
# End Source File
# Begin Source File

SOURCE=..\..\..\src\mbedtls\library\sha256.c
# End Source File
# Begin Source File

SOURCE=..\..\..\src\mbedtls\library\sha512.c
# End Source File
# Begin Source File

SOURCE=..\..\..\src\mbedtls\library\ssl_cache.c
# End Source File
# Begin Source File

SOURCE=..\..\..\src\mbedtls\library\ssl_ciphersuites.c
# End Source File
# Begin Source File

SOURCE=..\..\..\src\mbedtls\library\ssl_cli.c
# End Source File
# Begin Source File

SOURCE=..\..\..\src\mbedtls\library\ssl_cookie.c
# End Source File
# Begin Source File

SOURCE=..\..\..\src\mbedtls\library\ssl_srv.c
# End Source File
# Begin Source File

SOURCE=..\..\..\src\mbedtls\library\ssl_ticket.c
# End Source File
# Begin Source File

SOURCE=..\..\..\src\mbedtls\library\ssl_tls.c
# End Source File
# Begin Source File

SOURCE=..\..\..\src\mbedtls\library\threading.c
# End Source File
# Begin Source File

SOURCE=..\..\..\src\mbedtls\library\timing.c
# End Source File
# Begin Source File

SOURCE=..\..\..\src\mbedtls\library\version.c
# End Source File
# Begin Source File

SOURCE=..\..\..\src\mbedtls\library\version_features.c
# End Source File
# Begin Source File

SOURCE=..\..\..\src\mbedtls\library\x509.c
# End Source File
# Begin Source File

SOURCE=..\..\..\src\mbedtls\library\x509_create.c
# End Source File
# Begin Source File

SOURCE=..\..\..\src\mbedtls\library\x509_crl.c
# End Source File
# Begin Source File

SOURCE=..\..\..\src\mbedtls\library\x509_crt.c
# End Source File
# Begin Source File

SOURCE=..\..\..\src\mbedtls\library\x509_csr.c
# End Source File
# Begin Source File

SOURCE=..\..\..\src\mbedtls\library\x509write_crt.c
# End Source File
# Begin Source File

SOURCE=..\..\..\src\mbedtls\library\x509write_csr.c
# End Source File
# Begin Source File

SOURCE=..\..\..\src\mbedtls\library\xtea.c
# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl"
# Begin Source File

SOURCE=..\..\..\src\mbedtls\include\mbedtls\aes.h
# End Source File
# Begin Source File

SOURCE=..\..\..\src\mbedtls\include\mbedtls\aesni.h
# End Source File
# Begin Source File

SOURCE=..\..\..\src\mbedtls\include\mbedtls\arc4.h
# End Source File
# Begin Source File

SOURCE=..\..\..\src\mbedtls\include\mbedtls\aria.h
# End Source File
# Begin Source File

SOURCE=..\..\..\src\mbedtls\include\mbedtls\asn1.h
# End Source File
# Begin Source File

SOURCE=..\..\..\src\mbedtls\include\mbedtls\asn1write.h
# End Source File
# Begin Source File

SOURCE=..\..\..\src\mbedtls\include\mbedtls\base64.h
# End Source File
# Begin Source File

SOURCE=..\..\..\src\mbedtls\include\mbedtls\bignum.h
# End Source File
# Begin Source File

SOURCE=..\..\..\src\mbedtls\include\mbedtls\blowfish.h
# End Source File
# Begin Source File

SOURCE=..\..\..\src\mbedtls\include\mbedtls\bn_mul.h
# End Source File
# Begin Source File

SOURCE=..\..\..\src\mbedtls\include\mbedtls\camellia.h
# End Source File
# Begin Source File

SOURCE=..\..\..\src\mbedtls\include\mbedtls\ccm.h
# End Source File
# Begin Source File

SOURCE=..\..\..\src\mbedtls\include\mbedtls\certs.h
# End Source File
# Begin Source File

SOURCE=..\..\..\src\mbedtls\include\mbedtls\chacha20.h
# End Source File
# Begin Source File

SOURCE=..\..\..\src\mbedtls\include\mbedtls\chachapoly.h
# End Source File
# Begin Source File

SOURCE=..\..\..\src\mbedtls\include\mbedtls\check_config.h
# End Source File
# Begin Source File

SOURCE=..\..\..\src\mbedtls\include\mbedtls\cipher.h
# End Source File
# Begin Source File

SOURCE=..\..\..\src\mbedtls\include\mbedtls\cipher_internal.h
# End Source File
# Begin Source File

SOURCE=..\..\..\src\mbedtls\include\mbedtls\cmac.h
# End Source File
# Begin Source File

SOURCE="..\..\..\src\mbedtls\include\mbedtls\compat-1.3.h"
# End Source File
# Begin Source File

SOURCE=..\..\..\src\mbedtls\include\mbedtls\config.h
# End Source File
# Begin Source File

SOURCE=..\..\..\src\mbedtls\include\mbedtls\ctr_drbg.h
# End Source File
# Begin Source File

SOURCE=..\..\..\src\mbedtls\include\mbedtls\debug.h
# End Source File
# Begin Source File

SOURCE=..\..\..\src\mbedtls\include\mbedtls\des.h
# End Source File
# Begin Source File

SOURCE=..\..\..\src\mbedtls\include\mbedtls\dhm.h
# End Source File
# Begin Source File

SOURCE=..\..\..\src\mbedtls\include\mbedtls\ecdh.h
# End Source File
# Begin Source File

SOURCE=..\..\..\src\mbedtls\include\mbedtls\ecdsa.h
# End Source File
# Begin Source File

SOURCE=..\..\..\src\mbedtls\include\mbedtls\ecjpake.h
# End Source File
# Begin Source File

SOURCE=..\..\..\src\mbedtls\include\mbedtls\ecp.h
# End Source File
# Begin Source File

SOURCE=..\..\..\src\mbedtls\include\mbedtls\ecp_internal.h
# End Source File
# Begin Source File

SOURCE=..\..\..\src\mbedtls\include\mbedtls\entropy.h
# End Source File
# Begin Source File

SOURCE=..\..\..\src\mbedtls\include\mbedtls\entropy_poll.h
# End Source File
# Begin Source File

SOURCE=..\..\..\src\mbedtls\include\mbedtls\error.h
# End Source File
# Begin Source File

SOURCE=..\..\..\src\mbedtls\include\mbedtls\gcm.h
# End Source File
# Begin Source File

SOURCE=..\..\..\src\mbedtls\include\mbedtls\havege.h
# End Source File
# Begin Source File

SOURCE=..\..\..\src\mbedtls\include\mbedtls\hkdf.h
# End Source File
# Begin Source File

SOURCE=..\..\..\src\mbedtls\include\mbedtls\hmac_drbg.h
# End Source File
# Begin Source File

SOURCE=..\..\..\src\mbedtls\include\mbedtls\md.h
# End Source File
# Begin Source File

SOURCE=..\..\..\src\mbedtls\include\mbedtls\md2.h
# End Source File
# Begin Source File

SOURCE=..\..\..\src\mbedtls\include\mbedtls\md4.h
# End Source File
# Begin Source File

SOURCE=..\..\..\src\mbedtls\include\mbedtls\md5.h
# End Source File
# Begin Source File

SOURCE=..\..\..\src\mbedtls\include\mbedtls\md_internal.h
# End Source File
# Begin Source File

SOURCE=..\..\..\src\mbedtls\include\mbedtls\memory_buffer_alloc.h
# End Source File
# Begin Source File

SOURCE=..\..\..\src\mbedtls\include\mbedtls\net.h
# End Source File
# Begin Source File

SOURCE=..\..\..\src\mbedtls\include\mbedtls\net_sockets.h
# End Source File
# Begin Source File

SOURCE=..\..\..\src\mbedtls\include\mbedtls\nist_kw.h
# End Source File
# Begin Source File

SOURCE=..\..\..\src\mbedtls\include\mbedtls\oid.h
# End Source File
# Begin Source File

SOURCE=..\..\..\src\mbedtls\include\mbedtls\padlock.h
# End Source File
# Begin Source File

SOURCE=..\..\..\src\mbedtls\include\mbedtls\pem.h
# End Source File
# Begin Source File

SOURCE=..\..\..\src\mbedtls\include\mbedtls\pk.h
# End Source File
# Begin Source File

SOURCE=..\..\..\src\mbedtls\include\mbedtls\pk_internal.h
# End Source File
# Begin Source File

SOURCE=..\..\..\src\mbedtls\include\mbedtls\pkcs11.h
# End Source File
# Begin Source File

SOURCE=..\..\..\src\mbedtls\include\mbedtls\pkcs12.h
# End Source File
# Begin Source File

SOURCE=..\..\..\src\mbedtls\include\mbedtls\pkcs5.h
# End Source File
# Begin Source File

SOURCE=..\..\..\src\mbedtls\include\mbedtls\platform.h
# End Source File
# Begin Source File

SOURCE=..\..\..\src\mbedtls\include\mbedtls\platform_time.h
# End Source File
# Begin Source File

SOURCE=..\..\..\src\mbedtls\include\mbedtls\platform_util.h
# End Source File
# Begin Source File

SOURCE=..\..\..\src\mbedtls\include\mbedtls\poly1305.h
# End Source File
# Begin Source File

SOURCE=..\..\..\src\mbedtls\include\mbedtls\ripemd160.h
# End Source File
# Begin Source File

SOURCE=..\..\..\src\mbedtls\include\mbedtls\rsa.h
# End Source File
# Begin Source File

SOURCE=..\..\..\src\mbedtls\include\mbedtls\rsa_internal.h
# End Source File
# Begin Source File

SOURCE=..\..\..\src\mbedtls\include\mbedtls\sha1.h
# End Source File
# Begin Source File

SOURCE=..\..\..\src\mbedtls\include\mbedtls\sha256.h
# End Source File
# Begin Source File

SOURCE=..\..\..\src\mbedtls\include\mbedtls\sha512.h
# End Source File
# Begin Source File

SOURCE=..\..\..\src\mbedtls\include\mbedtls\ssl.h
# End Source File
# Begin Source File

SOURCE=..\..\..\src\mbedtls\include\mbedtls\ssl_cache.h
# End Source File
# Begin Source File

SOURCE=..\..\..\src\mbedtls\include\mbedtls\ssl_ciphersuites.h
# End Source File
# Begin Source File

SOURCE=..\..\..\src\mbedtls\include\mbedtls\ssl_cookie.h
# End Source File
# Begin Source File

SOURCE=..\..\..\src\mbedtls\include\mbedtls\ssl_internal.h
# End Source File
# Begin Source File

SOURCE=..\..\..\src\mbedtls\include\mbedtls\ssl_ticket.h
# End Source File
# Begin Source File

SOURCE=..\..\..\src\mbedtls\include\mbedtls\threading.h
# End Source File
# Begin Source File

SOURCE=..\..\..\src\mbedtls\include\mbedtls\threading_alt.h
# End Source File
# Begin Source File

SOURCE=..\..\..\src\mbedtls\include\mbedtls\timing.h
# End Source File
# Begin Source File

SOURCE=..\..\..\src\mbedtls\include\mbedtls\version.h
# End Source File
# Begin Source File

SOURCE=..\..\..\src\mbedtls\include\mbedtls\x509.h
# End Source File
# Begin Source File

SOURCE=..\..\..\src\mbedtls\include\mbedtls\x509_crl.h
# End Source File
# Begin Source File

SOURCE=..\..\..\src\mbedtls\include\mbedtls\x509_crt.h
# End Source File
# Begin Source File

SOURCE=..\..\..\src\mbedtls\include\mbedtls\x509_csr.h
# End Source File
# Begin Source File

SOURCE=..\..\..\src\mbedtls\include\mbedtls\xtea.h
# End Source File
# End Group
# End Target
# End Project
