# Microsoft Developer Studio Project File - Name="RayForce" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Static Library" 0x0104

CFG=RayForce - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "RayForce.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "RayForce.mak" CFG="RayForce - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "RayForce - Win32 Release" (based on "Win32 (x86) Static Library")
!MESSAGE "RayForce - Win32 Debug" (based on "Win32 (x86) Static Library")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe
RSC=rc.exe

!IF  "$(CFG)" == "RayForce - Win32 Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Release"
# PROP BASE Intermediate_Dir "Release"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "Release"
# PROP Intermediate_Dir "Release"
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_MBCS" /D "_LIB" /YX /FD /c
# ADD CPP /nologo /MT /W3 /GX- /O2 /D "WIN32" /D "NDEBUG" /D "_LIB" /D "UNICODE" /D "_UNICODE" /YX /FD /Zm800 /c
# ADD BASE RSC /l 0x804 /d "NDEBUG"
# ADD RSC /l 0x804 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LIB32=link.exe -lib
# ADD BASE LIB32 /nologo
# ADD LIB32 /nologo

!ELSEIF  "$(CFG)" == "RayForce - Win32 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Debug"
# PROP BASE Intermediate_Dir "Debug"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "Debug"
# PROP Intermediate_Dir "Debug"
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_MBCS" /D "_LIB" /YX /FD /GZ /c
# ADD CPP /nologo /MT /W3 /Gm /GX- /ZI /Od /D "WIN32" /D "_DEBUG" /D "_MBCS" /D "_LIB" /YX /FD /GZ /Zm800 /c
# ADD BASE RSC /l 0x804 /d "_DEBUG"
# ADD RSC /l 0x804 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LIB32=link.exe -lib
# ADD BASE LIB32 /nologo
# ADD LIB32 /nologo

!ENDIF 

# Begin Target

# Name "RayForce - Win32 Release"
# Name "RayForce - Win32 Debug"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Source File

SOURCE=..\..\..\..\Source\Utility\RayForce\Bridge.cpp
# End Source File
# Begin Source File

SOURCE=..\..\..\..\Source\Utility\RayForce\COM\ComBridge.cpp
# End Source File
# Begin Source File

SOURCE=..\..\..\..\Source\Utility\RayForce\COM\ComDef.cpp
# End Source File
# Begin Source File

SOURCE=..\..\..\..\Source\Utility\RayForce\COM\ComDispatch.cpp
# End Source File
# Begin Source File

SOURCE=..\..\..\..\Source\Utility\RayForce\DirectScript\DirectScriptBridge.cpp
# End Source File
# Begin Source File

SOURCE=..\..\..\..\Source\Utility\RayForce\LibLoader.cpp
# End Source File
# Begin Source File

SOURCE=..\..\..\..\Source\Utility\RayForce\ObjectDumper.cpp
# End Source File
# Begin Source File

SOURCE=..\..\..\..\Source\Utility\RayForce\Proxy.cpp
# End Source File
# Begin Source File

SOURCE=..\..\..\..\Source\Utility\RayForce\RayForce.cpp
# End Source File
# Begin Source File

SOURCE=..\..\..\..\Source\Utility\RayForce\Tunnel.cpp
# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl"
# Begin Source File

SOURCE=..\..\..\..\Source\Utility\RayForce\Bridge.h
# End Source File
# Begin Source File

SOURCE=..\..\..\..\Source\Utility\RayForce\COM\ComBridge.h
# End Source File
# Begin Source File

SOURCE=..\..\..\..\Source\Utility\RayForce\COM\ComDef.h
# End Source File
# Begin Source File

SOURCE=..\..\..\..\Source\Utility\RayForce\COM\ComDispatch.h
# End Source File
# Begin Source File

SOURCE=..\..\..\..\Source\Utility\RayForce\DirectScript\DirectScriptBridge.h
# End Source File
# Begin Source File

SOURCE=..\..\..\..\Source\Utility\RayForce\LibLoader.h
# End Source File
# Begin Source File

SOURCE=..\..\..\..\Source\Utility\RayForce\ObjectDumper.h
# End Source File
# Begin Source File

SOURCE=..\..\..\..\Source\Utility\RayForce\Proxy.h
# End Source File
# Begin Source File

SOURCE=..\..\..\..\Source\Utility\RayForce\RayForce.h
# End Source File
# Begin Source File

SOURCE=..\..\..\..\Source\Utility\RayForce\Tunnel.h
# End Source File
# End Group
# End Target
# End Project
