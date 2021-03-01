#include "ToolkitWin32.h"

#ifdef _WIN32
#include <Windows.h>
using namespace PaintsNow;

ToolkitWin32::ToolkitWin32() {}
ToolkitWin32::~ToolkitWin32() {}

TObject<IReflect>& ToolkitWin32::operator () (IReflect& reflect) {
	BaseClass::operator () (reflect);

	if (reflect.IsReflectMethod()) {
		ReflectMethod(RequestCreateProcess)[ScriptMethod = "CreateProcess"];
		ReflectMethod(RequestCloseHandle)[ScriptMethod = "CloseHandle"];
		ReflectMethod(RequestWaitForSingleObject)[ScriptMethod = "WaitForSingleObject"];
		ReflectMethod(RequestTerminateProcess)[ScriptMethod = "TerminateProcess"];
		ReflectMethod(RequestLoadLibrary)[ScriptMethod = "LoadLibrary"];
		ReflectMethod(RequestFreeLibrary)[ScriptMethod = "FreeLibrary"];
	}

	return *this;
}

uint64_t ToolkitWin32::RequestCreateProcess(IScript::Request& request, const String& path, const String& currentPath, const String& parameter) {
	String cmdLine = Utf8ToSystem(path);
	String directory = Utf8ToSystem(currentPath);
	STARTUPINFOW info = { 0 };
	PROCESS_INFORMATION pi = { 0 };

	if (::CreateProcessW(nullptr, (WCHAR*)cmdLine.c_str(), nullptr, nullptr, TRUE, 0, nullptr, currentPath.empty() ? nullptr : (WCHAR*)directory.c_str(), &info, &pi)) {
		::CloseHandle(pi.hThread);
		return (uint64_t)pi.hProcess;
	} else {
		return 0;
	}
}

void ToolkitWin32::RequestCloseHandle(IScript::Request& request, uint64_t object) {
	::CloseHandle((HANDLE)object);
}

uint32_t ToolkitWin32::RequestWaitForSingleObject(IScript::Request& request, uint64_t object, uint64_t timeout) {
	return ::WaitForSingleObject((HANDLE)object, timeout > 0xffffffff ? INFINITE : (DWORD)timeout);
}

bool ToolkitWin32::RequestTerminateProcess(IScript::Request& request, uint64_t process, uint32_t exitCode) {
	return ::TerminateProcess((HANDLE)process, exitCode) != 0;
}

uint64_t ToolkitWin32::RequestLoadLibrary(IScript::Request& request, const String& library) {
	return (uint64_t)::LoadLibraryW((const WCHAR*)Utf8ToSystem(library).c_str());
}

bool ToolkitWin32::RequestFreeLibrary(IScript::Request& request, uint64_t handle) {
	return ::FreeLibrary((HMODULE)handle) != 0;
}

#endif
