#include "LibLoader.h"


using namespace PaintsNow;

#ifdef __GNUC__
#define _stricmp strcasecmp
#endif

void* LibLoader::DynamicLoadLibrary(const String& path, const String& reason, void* param, IScript::Request* scriptRequest) {
	String last = path.substr(path.length() - 3, 3);
#if defined(__linux__)
	if (_stricmp(last.c_str(), ".so") == 0) {
		String p = Utf8ToSystem(path);
		printf("Ready to load %s\n", path.c_str()); 
		void* lib = dlopen(p.c_str(), RTLD_NOW | RTLD_GLOBAL);
		if (lib != nullptr) {
			printf("Load %s success.\n", path.c_str()); 
			LeavesMainPrototype proc = (LeavesMainPrototype)dlsym(lib, "LeavesMain");
			if (proc == nullptr || !proc(reason.c_str(), param, scriptRequest)) {
				dlclose(lib);
				lib = (void*)-1;
			}
		}

		return lib;
	}
#elif defined(_WIN32)
	if (_stricmp(last.c_str(), "dll") == 0) {
		// try to Load
		String mod = Utf8ToSystem(path);

		HMODULE module = GetModuleHandleW((LPCWSTR)mod.c_str());
		if (module == nullptr) {
			module = LoadLibraryW((LPCWSTR)mod.c_str());
		}

		if (module != nullptr) {
			LeavesMainPrototype proc = (LeavesMainPrototype)GetProcAddress(module, "LeavesMain");
			if (proc != nullptr) {
				if (!proc(reason.c_str(), param, scriptRequest)) {
					FreeLibrary(module);
				}

				return (void*)module;
			} else {
				if (scriptRequest != nullptr) {
					IScript::Request& request = *scriptRequest;
					request.Error(String("Not a valid PaintsNow extension: ") + path + "\n");
				}
			}
		}

		if (scriptRequest != nullptr) {
			IScript::Request& request = *scriptRequest;
			request.Error(String("Load module failed: ") + path + "\n");
		}

		return (void*)-1;
	}
#endif

	return nullptr;
}
