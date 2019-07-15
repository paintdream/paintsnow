// LibLoader.h
// By PaintDream (paintdream@paintdream.com)
// 2016-3-8
//

#ifndef __LIBLOADER_H__
#define __LIBLOADER_H__

#include "../../Core/Interface/IScript.h"

namespace PaintsNow {
	class IReflectObject;
#if defined(__linux__)
#include <dlfcn.h>
	typedef bool (*LeavesMainPrototype)(const char* key, void* param,  IScript::Request* scriptRequest);
#elif defined(_WIN32)
#include <windows.h>
	typedef bool (*LeavesMainPrototype)(const char* key, void* param, IScript::Request* scriptRequest);
#endif

	class LibLoader {
	public:
		static void* DynamicLoadLibrary(const String& path, const String& reason, void* param, IScript::Request* scriptRequest = nullptr);
	};
}


#endif // __LIBLOADER_H__