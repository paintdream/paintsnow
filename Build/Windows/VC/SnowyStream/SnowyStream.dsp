# Microsoft Developer Studio Project File - Name="SnowyStream" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Static Library" 0x0104

CFG=SnowyStream - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "SnowyStream.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "SnowyStream.mak" CFG="SnowyStream - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "SnowyStream - Win32 Release" (based on "Win32 (x86) Static Library")
!MESSAGE "SnowyStream - Win32 Debug" (based on "Win32 (x86) Static Library")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe
RSC=rc.exe

!IF  "$(CFG)" == "SnowyStream - Win32 Release"

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
# ADD CPP /nologo /MT /W3 /O2 /Ob2 /D "WIN32" /D "NDEBUG" /D "_UNICODE" /D "UNICODE" /D "_LIB" /YX /FD /Zm700 /c
# ADD BASE RSC /l 0x804 /d "NDEBUG"
# ADD RSC /l 0x804 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LIB32=link.exe -lib
# ADD BASE LIB32 /nologo
# ADD LIB32 /nologo

!ELSEIF  "$(CFG)" == "SnowyStream - Win32 Debug"

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
# ADD CPP /nologo /MT /W3 /Gm /ZI /Od /D "WIN32" /D "_DEBUG" /D "_MBCS" /D "_LIB" /YX /FD /GZ /Zm800 /c
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

# Name "SnowyStream - Win32 Release"
# Name "SnowyStream - Win32 Debug"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Source File

SOURCE=..\..\..\..\Source\Utility\SnowyStream\Resource\AnalyticCurveResource.cpp
# End Source File
# Begin Source File

SOURCE=..\..\..\..\Source\Utility\SnowyStream\Resource\Shaders\AntiAliasingFS.cpp
# End Source File
# Begin Source File

SOURCE=..\..\..\..\Source\Utility\SnowyStream\Resource\Passes\AntiAliasingPass.cpp
# End Source File
# Begin Source File

SOURCE=..\..\..\..\Source\Utility\SnowyStream\Resource\AudioResource.cpp
# End Source File
# Begin Source File

SOURCE=..\..\..\..\Source\Utility\SnowyStream\Resource\Shaders\BloomFS.cpp
# End Source File
# Begin Source File

SOURCE=..\..\..\..\Source\Utility\SnowyStream\Resource\Passes\BloomPass.cpp
# End Source File
# Begin Source File

SOURCE=..\..\..\..\Source\Utility\SnowyStream\Resource\Shaders\ConstMapFS.cpp
# End Source File
# Begin Source File

SOURCE=..\..\..\..\Source\Utility\SnowyStream\Resource\Passes\ConstMapPass.cpp
# End Source File
# Begin Source File

SOURCE=..\..\..\..\Source\Utility\SnowyStream\Resource\Passes\CustomizeShader.cpp
# End Source File
# Begin Source File

SOURCE=..\..\..\..\Source\Utility\SnowyStream\Resource\Shaders\CustomMaterialParameterFS.cpp
# End Source File
# Begin Source File

SOURCE=..\..\..\..\Source\Utility\SnowyStream\Resource\Passes\CustomMaterialPass.cpp
# End Source File
# Begin Source File

SOURCE=..\..\..\..\Source\Utility\SnowyStream\Resource\Shaders\DeferredCompactFS.cpp
# End Source File
# Begin Source File

SOURCE=..\..\..\..\Source\Utility\SnowyStream\Resource\Passes\DeferredLightingPass.cpp
# End Source File
# Begin Source File

SOURCE=..\..\..\..\Source\Utility\SnowyStream\Resource\Passes\DepthBoundingPass.cpp
# End Source File
# Begin Source File

SOURCE=..\..\..\..\Source\Utility\SnowyStream\Resource\Passes\DepthBoundingSetupPass.cpp
# End Source File
# Begin Source File

SOURCE=..\..\..\..\Source\Utility\SnowyStream\Resource\Shaders\DepthMinMaxFS.cpp
# End Source File
# Begin Source File

SOURCE=..\..\..\..\Source\Utility\SnowyStream\Resource\Shaders\DepthMinMaxSetupFS.cpp
# End Source File
# Begin Source File

SOURCE=..\..\..\..\Source\Utility\SnowyStream\Resource\Shaders\DepthResolveFS.cpp
# End Source File
# Begin Source File

SOURCE=..\..\..\..\Source\Utility\SnowyStream\Resource\Passes\DepthResolvePass.cpp
# End Source File
# Begin Source File

SOURCE=..\..\..\..\Source\Utility\SnowyStream\File.cpp
# End Source File
# Begin Source File

SOURCE=..\..\..\..\Source\Utility\SnowyStream\Resource\FontResource.cpp
# End Source File
# Begin Source File

SOURCE=..\..\..\..\Source\Utility\SnowyStream\Resource\Passes\ForwardLightingPass.cpp
# End Source File
# Begin Source File

SOURCE=..\..\..\..\Source\Utility\SnowyStream\Resource\GraphicResourceBase.cpp
# End Source File
# Begin Source File

SOURCE=..\..\..\..\Source\Utility\SnowyStream\Resource\Passes\LightBufferPass.cpp
# End Source File
# Begin Source File

SOURCE=..\..\..\..\Source\Utility\SnowyStream\Resource\Shaders\LightEncoderFS.cpp
# End Source File
# Begin Source File

SOURCE=..\..\..\..\Source\Utility\SnowyStream\Resource\MaterialResource.cpp
# End Source File
# Begin Source File

SOURCE=..\..\..\..\Source\Utility\SnowyStream\Resource\MeshResource.cpp
# End Source File
# Begin Source File

SOURCE=..\..\..\..\Source\Utility\SnowyStream\Resource\Shaders\MultiHashGatherFS.cpp
# End Source File
# Begin Source File

SOURCE=..\..\..\..\Source\Utility\SnowyStream\Resource\Passes\MultiHashGatherPass.cpp
# End Source File
# Begin Source File

SOURCE=..\..\..\..\Source\Utility\SnowyStream\Resource\Shaders\MultiHashLightFS.cpp
# End Source File
# Begin Source File

SOURCE=..\..\..\..\Source\Utility\SnowyStream\Resource\Passes\MultiHashLightPass.cpp
# End Source File
# Begin Source File

SOURCE=..\..\..\..\Source\Utility\SnowyStream\Resource\Shaders\MultiHashSetupFS.cpp
# End Source File
# Begin Source File

SOURCE=..\..\..\..\Source\Utility\SnowyStream\Resource\Passes\MultiHashSetupPass.cpp
# End Source File
# Begin Source File

SOURCE=..\..\..\..\Source\Utility\SnowyStream\Resource\Shaders\MultiHashTraceFS.cpp
# End Source File
# Begin Source File

SOURCE=..\..\..\..\Source\Utility\SnowyStream\Resource\Passes\MultiHashTracePass.cpp
# End Source File
# Begin Source File

SOURCE=..\..\..\..\Source\Utility\SnowyStream\Resource\Passes\ParticlePass.cpp
# End Source File
# Begin Source File

SOURCE=..\..\..\..\Source\Utility\SnowyStream\Resource\ParticleResource.cpp
# End Source File
# Begin Source File

SOURCE=..\..\..\..\Source\Utility\SnowyStream\ResourceBase.cpp
# End Source File
# Begin Source File

SOURCE=..\..\..\..\Source\Utility\SnowyStream\ResourceManager.cpp
# End Source File
# Begin Source File

SOURCE=..\..\..\..\Source\Utility\SnowyStream\Resource\Shaders\ScreenFS.cpp
# End Source File
# Begin Source File

SOURCE=..\..\..\..\Source\Utility\SnowyStream\Resource\Passes\ScreenPass.cpp
# End Source File
# Begin Source File

SOURCE=..\..\..\..\Source\Utility\SnowyStream\Resource\Shaders\ScreenSpaceTraceFS.cpp
# End Source File
# Begin Source File

SOURCE=..\..\..\..\Source\Utility\SnowyStream\Resource\Shaders\ScreenTransformVS.cpp
# End Source File
# Begin Source File

SOURCE=..\..\..\..\Source\Utility\SnowyStream\Resource\ShaderResource.cpp
# End Source File
# Begin Source File

SOURCE=..\..\..\..\Source\Utility\SnowyStream\Resource\Shaders\ShadowMapVS.cpp
# End Source File
# Begin Source File

SOURCE=..\..\..\..\Source\Utility\SnowyStream\Resource\SkeletonResource.cpp
# End Source File
# Begin Source File

SOURCE=..\..\..\..\Source\Utility\SnowyStream\SnowyStream.cpp
# End Source File
# Begin Source File

SOURCE=..\..\..\..\Source\Utility\SnowyStream\Resource\Shaders\StandardLightingFS.cpp
# End Source File
# Begin Source File

SOURCE=..\..\..\..\Source\Utility\SnowyStream\Resource\Shaders\StandardParameterFS.cpp
# End Source File
# Begin Source File

SOURCE=..\..\..\..\Source\Utility\SnowyStream\Resource\Passes\StandardPass.cpp
# End Source File
# Begin Source File

SOURCE=..\..\..\..\Source\Utility\SnowyStream\Resource\Shaders\StandardTransformVS.cpp
# End Source File
# Begin Source File

SOURCE=..\..\..\..\Source\Utility\SnowyStream\Resource\Shaders\SurfaceVS.cpp
# End Source File
# Begin Source File

SOURCE=..\..\..\..\Source\Utility\SnowyStream\Resource\TapeResource.cpp
# End Source File
# Begin Source File

SOURCE=..\..\..\..\Source\Utility\SnowyStream\Resource\Passes\TerrainPass.cpp
# End Source File
# Begin Source File

SOURCE=..\..\..\..\Source\Utility\SnowyStream\Resource\TerrainResource.cpp
# End Source File
# Begin Source File

SOURCE=..\..\..\..\Source\Utility\SnowyStream\Resource\TextResource.cpp
# End Source File
# Begin Source File

SOURCE=..\..\..\..\Source\Utility\SnowyStream\Resource\TextureResource.cpp
# End Source File
# Begin Source File

SOURCE=..\..\..\..\Source\Utility\SnowyStream\Resource\Shaders\TileBasedLightCS.cpp
# End Source File
# Begin Source File

SOURCE=..\..\..\..\Source\Utility\SnowyStream\Resource\Passes\VolumePass.cpp
# End Source File
# Begin Source File

SOURCE=..\..\..\..\Source\Utility\SnowyStream\Resource\VolumeResource.cpp
# End Source File
# Begin Source File

SOURCE=..\..\..\..\Source\Utility\SnowyStream\Resource\Passes\WaterPass.cpp
# End Source File
# Begin Source File

SOURCE=..\..\..\..\Source\Utility\SnowyStream\Resource\Passes\WidgetPass.cpp
# End Source File
# Begin Source File

SOURCE=..\..\..\..\Source\Utility\SnowyStream\Resource\Shaders\WidgetShadingFS.cpp
# End Source File
# Begin Source File

SOURCE=..\..\..\..\Source\Utility\SnowyStream\Resource\Shaders\WidgetTransformVS.cpp
# End Source File
# Begin Source File

SOURCE=..\..\..\..\Source\Utility\SnowyStream\Zipper.cpp
# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl"
# Begin Source File

SOURCE=..\..\..\..\Source\Utility\SnowyStream\Resource\AnalyticCurveResource.h
# End Source File
# Begin Source File

SOURCE=..\..\..\..\Source\Utility\SnowyStream\Resource\Shaders\AntiAliasingFS.h
# End Source File
# Begin Source File

SOURCE=..\..\..\..\Source\Utility\SnowyStream\Resource\Passes\AntiAliasingPass.h
# End Source File
# Begin Source File

SOURCE=..\..\..\..\Source\Utility\SnowyStream\Resource\AudioResource.h
# End Source File
# Begin Source File

SOURCE=..\..\..\..\Source\Utility\SnowyStream\Resource\Shaders\BloomFS.h
# End Source File
# Begin Source File

SOURCE=..\..\..\..\Source\Utility\SnowyStream\Resource\Passes\BloomPass.h
# End Source File
# Begin Source File

SOURCE=..\..\..\..\Source\Utility\SnowyStream\Resource\Shaders\ConstMapFS.h
# End Source File
# Begin Source File

SOURCE=..\..\..\..\Source\Utility\SnowyStream\Resource\Passes\ConstMapPass.h
# End Source File
# Begin Source File

SOURCE=..\..\..\..\Source\Utility\SnowyStream\Resource\Passes\CustomizeShader.h
# End Source File
# Begin Source File

SOURCE=..\..\..\..\Source\Utility\SnowyStream\Resource\Shaders\CustomMaterialParameterFS.h
# End Source File
# Begin Source File

SOURCE=..\..\..\..\Source\Utility\SnowyStream\Resource\Passes\CustomMaterialPass.h
# End Source File
# Begin Source File

SOURCE=..\..\..\..\Source\Utility\SnowyStream\Resource\Shaders\DeferredCompactFS.h
# End Source File
# Begin Source File

SOURCE=..\..\..\..\Source\Utility\SnowyStream\Resource\Passes\DeferredLightingPass.h
# End Source File
# Begin Source File

SOURCE=..\..\..\..\Source\Utility\SnowyStream\Resource\Passes\DepthBoundingPass.h
# End Source File
# Begin Source File

SOURCE=..\..\..\..\Source\Utility\SnowyStream\Resource\Passes\DepthBoundingSetupPass.h
# End Source File
# Begin Source File

SOURCE=..\..\..\..\Source\Utility\SnowyStream\Resource\Shaders\DepthMinMaxFS.h
# End Source File
# Begin Source File

SOURCE=..\..\..\..\Source\Utility\SnowyStream\Resource\Shaders\DepthMinMaxSetupFS.h
# End Source File
# Begin Source File

SOURCE=..\..\..\..\Source\Utility\SnowyStream\Resource\Shaders\DepthResolveFS.h
# End Source File
# Begin Source File

SOURCE=..\..\..\..\Source\Utility\SnowyStream\Resource\Passes\DepthResolvePass.h
# End Source File
# Begin Source File

SOURCE=..\..\..\..\Source\Utility\SnowyStream\File.h
# End Source File
# Begin Source File

SOURCE=..\..\..\..\Source\Utility\SnowyStream\Resource\FontResource.h
# End Source File
# Begin Source File

SOURCE=..\..\..\..\Source\Utility\SnowyStream\Resource\Passes\ForwardLightingPass.h
# End Source File
# Begin Source File

SOURCE=..\..\..\..\Source\Utility\SnowyStream\Resource\GraphicResourceBase.h
# End Source File
# Begin Source File

SOURCE=..\..\..\..\Source\Utility\SnowyStream\Resource\Passes\LightBufferPass.h
# End Source File
# Begin Source File

SOURCE=..\..\..\..\Source\Utility\SnowyStream\Resource\Shaders\LightEncoderFS.h
# End Source File
# Begin Source File

SOURCE=..\..\..\..\Source\Utility\SnowyStream\Resource\MaterialResource.h
# End Source File
# Begin Source File

SOURCE=..\..\..\..\Source\Utility\SnowyStream\Resource\MeshResource.h
# End Source File
# Begin Source File

SOURCE=..\..\..\..\Source\Utility\SnowyStream\Resource\Shaders\MultiHashGatherFS.h
# End Source File
# Begin Source File

SOURCE=..\..\..\..\Source\Utility\SnowyStream\Resource\Passes\MultiHashGatherPass.h
# End Source File
# Begin Source File

SOURCE=..\..\..\..\Source\Utility\SnowyStream\Resource\Shaders\MultiHashLightFS.h
# End Source File
# Begin Source File

SOURCE=..\..\..\..\Source\Utility\SnowyStream\Resource\Passes\MultiHashLightPass.h
# End Source File
# Begin Source File

SOURCE=..\..\..\..\Source\Utility\SnowyStream\Resource\Shaders\MultiHashSetupFS.h
# End Source File
# Begin Source File

SOURCE=..\..\..\..\Source\Utility\SnowyStream\Resource\Passes\MultiHashSetupPass.h
# End Source File
# Begin Source File

SOURCE=..\..\..\..\Source\Utility\SnowyStream\Resource\Shaders\MultiHashTraceFS.h
# End Source File
# Begin Source File

SOURCE=..\..\..\..\Source\Utility\SnowyStream\Resource\Passes\MultiHashTracePass.h
# End Source File
# Begin Source File

SOURCE=..\..\..\..\Source\Utility\SnowyStream\Resource\Passes\ParticlePass.h
# End Source File
# Begin Source File

SOURCE=..\..\..\..\Source\Utility\SnowyStream\Resource\ParticleResource.h
# End Source File
# Begin Source File

SOURCE=..\..\..\..\Source\Utility\SnowyStream\ResourceBase.h
# End Source File
# Begin Source File

SOURCE=..\..\..\..\Source\Utility\SnowyStream\ResourceManager.h
# End Source File
# Begin Source File

SOURCE=..\..\..\..\Source\Utility\SnowyStream\Resource\Shaders\ScreenFS.h
# End Source File
# Begin Source File

SOURCE=..\..\..\..\Source\Utility\SnowyStream\Resource\Passes\ScreenPass.h
# End Source File
# Begin Source File

SOURCE=..\..\..\..\Source\Utility\SnowyStream\Resource\Shaders\ScreenSpaceTraceFS.h
# End Source File
# Begin Source File

SOURCE=..\..\..\..\Source\Utility\SnowyStream\Resource\Shaders\ScreenTransformVS.h
# End Source File
# Begin Source File

SOURCE=..\..\..\..\Source\Utility\SnowyStream\Resource\ShaderResource.h
# End Source File
# Begin Source File

SOURCE=..\..\..\..\Source\Utility\SnowyStream\Resource\Shaders\ShadowMapVS.h
# End Source File
# Begin Source File

SOURCE=..\..\..\..\Source\Utility\SnowyStream\Resource\SkeletonResource.h
# End Source File
# Begin Source File

SOURCE=..\..\..\..\Source\Utility\SnowyStream\SnowyStream.h
# End Source File
# Begin Source File

SOURCE=..\..\..\..\Source\Utility\SnowyStream\Resource\Shaders\StandardLightingFS.h
# End Source File
# Begin Source File

SOURCE=..\..\..\..\Source\Utility\SnowyStream\Resource\Shaders\StandardParameterFS.h
# End Source File
# Begin Source File

SOURCE=..\..\..\..\Source\Utility\SnowyStream\Resource\Passes\StandardPass.h
# End Source File
# Begin Source File

SOURCE=..\..\..\..\Source\Utility\SnowyStream\Resource\Shaders\StandardTransformVS.h
# End Source File
# Begin Source File

SOURCE=..\..\..\..\Source\Utility\SnowyStream\Resource\Shaders\SurfaceVS.h
# End Source File
# Begin Source File

SOURCE=..\..\..\..\Source\Utility\SnowyStream\Resource\TapeResource.h
# End Source File
# Begin Source File

SOURCE=..\..\..\..\Source\Utility\SnowyStream\Resource\Passes\TerrainPass.h
# End Source File
# Begin Source File

SOURCE=..\..\..\..\Source\Utility\SnowyStream\Resource\TerrainResource.h
# End Source File
# Begin Source File

SOURCE=..\..\..\..\Source\Utility\SnowyStream\Resource\TextResource.h
# End Source File
# Begin Source File

SOURCE=..\..\..\..\Source\Utility\SnowyStream\Resource\TextureResource.h
# End Source File
# Begin Source File

SOURCE=..\..\..\..\Source\Utility\SnowyStream\Resource\Shaders\TileBasedLightCS.h
# End Source File
# Begin Source File

SOURCE=..\..\..\..\Source\Utility\SnowyStream\Resource\Passes\VolumePass.h
# End Source File
# Begin Source File

SOURCE=..\..\..\..\Source\Utility\SnowyStream\Resource\VolumeResource.h
# End Source File
# Begin Source File

SOURCE=..\..\..\..\Source\Utility\SnowyStream\Resource\Passes\WaterPass.h
# End Source File
# Begin Source File

SOURCE=..\..\..\..\Source\Utility\SnowyStream\Resource\Passes\WidgetPass.h
# End Source File
# Begin Source File

SOURCE=..\..\..\..\Source\Utility\SnowyStream\Resource\Shaders\WidgetShadingFS.h
# End Source File
# Begin Source File

SOURCE=..\..\..\..\Source\Utility\SnowyStream\Resource\Shaders\WidgetTransformVS.h
# End Source File
# Begin Source File

SOURCE=..\..\..\..\Source\Utility\SnowyStream\Zipper.h
# End Source File
# End Group
# End Target
# End Project
