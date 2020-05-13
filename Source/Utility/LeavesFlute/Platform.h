// Platform.h
// PaintDream (paintdream@paintdream.com)
// 2018-8-24
//

#ifndef __PLATFORM_H__
#define __PLATFORM_H__

#include "../../Core/PaintsNow.h"
#define PAINTSNOW_VERSION_MAJOR "0"
#define PAINTSNOW_VERSION_MINOR "0.20.1.1 [Dev]"

#ifdef _MSC_VER
#if !defined(CMAKE_PAINTSNOW) || ADD_AUDIO_LAME
#if defined(_DEBUG) && _MSC_VER > 1200
#pragma comment(lib, "libmp3lameD.lib")
#pragma comment(lib, "mpghipD.lib")
#else
#pragma comment(lib, "libmp3lame.lib")
#pragma comment(lib, "mpghip.lib")
#endif
#endif

#if !defined(CMAKE_PAINTSNOW) || ADD_RENDER_OPENGL
#if defined(_DEBUG) && _MSC_VER > 1200
#pragma comment(lib, "glew32sd.lib")
#else
#pragma comment(lib, "glew32s.lib")
#endif
#endif

#if !defined(CMAKE_PAINTSNOW) || ADD_AUDIO_OPENAL
#if defined(_DEBUG) && _MSC_VER > 1200
#pragma comment(lib, "OpenAL32D.lib")
#else
#pragma comment(lib, "OpenAL32.lib")
#endif
#endif

#if !defined(CMAKE_PAINTSNOW) || ADD_FONT_FREETYPE
#if defined(_DEBUG) && _MSC_VER > 1200
#pragma comment(lib, "freetypeD.lib")
#else
#pragma comment(lib, "freetype.lib")
#endif
#endif

#if !defined(CMAKE_PAINTSNOW) || ADD_IMAGE_FREEIMAGE
#if defined(_DEBUG) && _MSC_VER > 1200
#pragma comment(lib, "freeimageD.lib")
#else
#pragma comment(lib, "freeimage.lib")
#endif
#endif

#if !defined(CMAKE_PAINTSNOW) || ADD_FRAME_FREEGLUT
#if defined(_DEBUG) && _MSC_VER > 1200
#pragma comment(lib, "freeglut_staticD.lib")
#else
#pragma comment(lib, "freeglut_static.lib")
#endif
#endif

#if !defined(CMAKE_PAINTSNOW) || ADD_FRAME_DXUT_BUILTIN
#if defined(CMAKE_PAINTSNOW)
#define USE_DIRECTX9
#if USE_STATIC_THIRDPARTY_LIBRARIES
#if defined(_DEBUG) && _MSC_VER > 1200
#pragma comment(lib, "d3dx9d.lib")
#pragma comment(lib, "d3dx10d.lib")
#else
#pragma comment(lib, "d3dx9.lib")
#pragma comment(lib, "d3dx10.lib")
#endif
#pragma comment(lib, "dxerr.lib")
#pragma comment(lib, "dxguid.lib")
#endif
#endif
#endif

#if !defined(_MSC_VER) || _MSC_VER <= 1200
#if !defined(CMAKE_PAINTSNOW) || ADD_THREAD_PTHREAD
#if defined(_DEBUG) && _MSC_VER > 1200
#pragma comment(lib, "pthreadD.lib")
#else
#pragma comment(lib, "pthread.lib")
#endif
#endif
#endif

#if !defined(CMAKE_PAINTSNOW) || ADD_NETWORK_LIBEVENT
#if defined(_DEBUG) && _MSC_VER > 1200
#pragma comment(lib, "libeventD.lib")
#else
#pragma comment(lib, "libevent.lib")
#endif
#endif

#endif


#if defined(_MSC_VER) && _MSC_VER <= 1200
#ifdef _DEBUG
#pragma comment(lib, "../../../../Build/Windows/VC/PaintsNow/Debug/PaintsNow.lib")
#pragma comment(lib, "../../../../Build/Windows/VC/HeartVioliner/Debug/HeartVioliner.lib")
#pragma comment(lib, "../../../../Build/Windows/VC/FlameWork/Debug/FlameWork.lib")
#pragma comment(lib, "../../../../Build/Windows/VC/SnowyStream/Debug/SnowyStream.lib")
#pragma comment(lib, "../../../../Build/Windows/VC/MythForest/Debug/MythForest.lib")
#pragma comment(lib, "../../../../Build/Windows/VC/BridgeSunset/Debug/BridgeSunset.lib")
#pragma comment(lib, "../../../../Build/Windows/VC/LeavesFlute/Debug/LeavesFlute.lib")
#pragma comment(lib, "../../../../Build/Windows/VC/EchoLegend/Debug/EchoLegend.lib")
#pragma comment(lib, "../../../../Build/Windows/VC/Remembery/Debug/Remembery.lib")
#pragma comment(lib, "../../../../Build/Windows/VC/GalaxyWeaver/Debug/GalaxyWeaver.lib")
#else
#pragma comment(lib, "../../../../Build/Windows/VC/PaintsNow/Release/PaintsNow.lib")
#pragma comment(lib, "../../../../Build/Windows/VC/HeartVioliner/Release/HeartVioliner.lib")
#pragma comment(lib, "../../../../Build/Windows/VC/FlameWork/Release/FlameWork.lib")
#pragma comment(lib, "../../../../Build/Windows/VC/SnowyStream/Release/SnowyStream.lib")
#pragma comment(lib, "../../../../Build/Windows/VC/MythForest/Release/MythForest.lib")
#pragma comment(lib, "../../../../Build/Windows/VC/BridgeSunset/Release/BridgeSunset.lib")
#pragma comment(lib, "../../../../Build/Windows/VC/LeavesFlute/Release/LeavesFlute.lib")
#pragma comment(lib, "../../../../Build/Windows/VC/EchoLegend/Release/EchoLegend.lib")
#pragma comment(lib, "../../../../Build/Windows/VC/Remembery/Release/Remembery.lib")
#pragma comment(lib, "../../../../Build/Windows/VC/GalaxyWeaver/Release/GalaxyWeaver.lib")
#endif
#endif // _MSC_VER

#endif