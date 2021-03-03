#include "ToolkitWin32.h"

#ifdef _WIN32
#include <Windows.h>
using namespace PaintsNow;

ToolkitWin32::ToolkitWin32() {
	mainThreadID = ::GetCurrentThreadId();
}

ToolkitWin32::~ToolkitWin32() {}

uint32_t ToolkitWin32::GetMainThreadID() const {
	return mainThreadID;
}

void ToolkitWin32::ScriptInitialize(IScript::Request& request) {
}

void ToolkitWin32::ScriptUninitialize(IScript::Request& request) {
	if (messageListener) {
		request.DoLock();
		request.Dereference(messageListener);
		request.UnLock();
	}
}

TObject<IReflect>& ToolkitWin32::operator () (IReflect& reflect) {
	BaseClass::operator () (reflect);

	if (reflect.IsReflectMethod()) {
		ReflectMethod(RequestExit)[ScriptMethod = "Exit"];
		ReflectMethod(RequestListenMessage)[ScriptMethod = "ListenMessage"];
		ReflectMethod(RequestPostThreadMessage)[ScriptMethod = "PostThreadMessage"];
		ReflectMethod(RequestCreateProcess)[ScriptMethod = "CreateProcess"];
		ReflectMethod(RequestCloseHandle)[ScriptMethod = "CloseHandle"];
		ReflectMethod(RequestWaitForSingleObject)[ScriptMethod = "WaitForSingleObject"];
		ReflectMethod(RequestTerminateProcess)[ScriptMethod = "TerminateProcess"];
		ReflectMethod(RequestLoadLibrary)[ScriptMethod = "LoadLibrary"];
		ReflectMethod(RequestFreeLibrary)[ScriptMethod = "FreeLibrary"];
	}

	return *this;
}

void ToolkitWin32::HandleMessage(LeavesFlute& flute, uint32_t msg, uint64_t wParam, uint64_t lParam) {
	if (messageListener) {
		IScript::Request& request = *flute.bridgeSunset.requestPool.AcquireSafe();
		request.DoLock();
		request.Call(messageListener, msg, wParam, lParam);
		request.UnLock();
		flute.bridgeSunset.requestPool.ReleaseSafe(&request);
	}
}

void ToolkitWin32::RequestListenMessage(IScript::Request& request, IScript::Request::Ref callback) {
	if (messageListener) {
		request.DoLock();
		request.Dereference(messageListener);
		request.UnLock();
	}

	messageListener = callback;
}

void ToolkitWin32::RequestExit(IScript::Request& request) {
	::PostThreadMessageA(mainThreadID, WM_QUIT, 0, 0);
}

void ToolkitWin32::RequestPostThreadMessage(IScript::Request& request, uint64_t thread, uint32_t msg, uint64_t wParam, uint64_t lParam) {
	::PostThreadMessageA((DWORD)thread, (UINT)msg, (WPARAM)wParam, (LPARAM)lParam);
}

std::pair<uint64_t, uint64_t> ToolkitWin32::RequestCreateProcess(IScript::Request& request, const String& path, const String& currentPath, const String& parameter) {
	String cmdLine = Utf8ToSystem(path);
	String directory = Utf8ToSystem(currentPath);
	STARTUPINFOW info = { 0 };
	info.cb = sizeof(STARTUPINFOW);
	info.dwFlags = STARTF_USESHOWWINDOW;
	info.wShowWindow = SW_HIDE; // default to hide
	PROCESS_INFORMATION pi = { 0 };

	if (::CreateProcessW(nullptr, (WCHAR*)cmdLine.c_str(), nullptr, nullptr, TRUE, 0, nullptr, currentPath.empty() ? nullptr : (WCHAR*)directory.c_str(), &info, &pi)) {
		::CloseHandle(pi.hThread);
		return std::make_pair((uint64_t)pi.hProcess, (uint64_t)pi.dwThreadId);
	} else {
		return std::make_pair(0, 0);
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
