#include "ServiceWin32.h"
#include "../../../LeavesFlute/Loader.h"

#ifdef _WIN32
using namespace PaintsNow;

ServiceWin32::ServiceWin32(const String& name) : serviceName(name), eventStop(nullptr) {}

ServiceWin32& ServiceWin32::GetInstance() {
	static ServiceWin32 theServiceWin32("LeavesWing");
	return theServiceWin32;
}

static void WINAPI ServiceCtrlHandler(DWORD opcode) {
	ServiceWin32::GetInstance().ServiceCtrlHandler(opcode);
}

__declspec(dllexport) void WINAPI ServiceMain(DWORD argc, LPTSTR* argv) {
	ServiceWin32::GetInstance().ServiceMain(argc, argv);
}

void ServiceWin32::ConsoleHandler(LeavesFlute& leavesFlute) {
	// TODO: Polling service?
	assert(eventStop != nullptr);
	::WaitForSingleObject(eventStop, INFINITE);
}

void ServiceWin32::ServiceMain(DWORD argc, LPTSTR* argv) {
	eventStop = ::CreateEventW(nullptr, FALSE, FALSE, nullptr);
	serviceStatus.dwServiceType = SERVICE_WIN32;
	serviceStatus.dwCurrentState = SERVICE_START_PENDING;
	serviceStatus.dwControlsAccepted = SERVICE_ACCEPT_STOP;
	serviceStatus.dwWin32ExitCode = 0;
	serviceStatus.dwServiceSpecificExitCode = 0;
	serviceStatus.dwCheckPoint = 0;
	serviceStatus.dwWaitHint = 0;

	String name = Utf8ToSystem(serviceName);
	serviceStatusHandle = ::RegisterServiceCtrlHandlerW((WCHAR*)name.c_str(), ::ServiceCtrlHandler);
	if (serviceStatusHandle != (SERVICE_STATUS_HANDLE)0) {
		serviceStatus.dwCurrentState = SERVICE_RUNNING;
		serviceStatus.dwCheckPoint = 0;
		serviceStatus.dwWaitHint = 0;
		::SetServiceStatus(serviceStatusHandle, &serviceStatus);
	}

	CmdLine cmdLine;
	cmdLine.Process((int)argc, argv);

	Loader loader;
	loader.consoleHandler = Wrap(this, &ServiceWin32::ConsoleHandler);
	loader.Run(cmdLine);
	::CloseHandle(eventStop);
	eventStop = nullptr;
}

void ServiceWin32::ServiceCtrlHandler(DWORD opcode) {
	switch (opcode) {
	case SERVICE_CONTROL_PAUSE:
		serviceStatus.dwCurrentState = SERVICE_PAUSED;
		break;
	case SERVICE_CONTROL_CONTINUE:
		serviceStatus.dwCurrentState = SERVICE_RUNNING;
		break;
	case SERVICE_CONTROL_STOP:
		serviceStatus.dwWin32ExitCode = 0;
		serviceStatus.dwCurrentState = SERVICE_STOPPED;
		serviceStatus.dwCheckPoint = 0;
		serviceStatus.dwWaitHint = 0;
		::SetServiceStatus(serviceStatusHandle, &serviceStatus);
		assert(eventStop != nullptr);
		::SetEvent(eventStop);
		break;
	case SERVICE_CONTROL_INTERROGATE:
		break;
	}
}

bool ServiceWin32::InstallService() {
	WCHAR fullPath[MAX_PATH * 2];
	::GetModuleFileNameW(nullptr, fullPath, MAX_PATH * 2);
	SC_HANDLE scManager = ::OpenSCManagerW(nullptr, nullptr, SC_MANAGER_ALL_ACCESS);
	if (scManager == nullptr) return false;

	String name = Utf8ToSystem(serviceName);
	SC_HANDLE scService = ::CreateServiceW(scManager, (WCHAR*)name.c_str(), (WCHAR*)name.c_str(), SERVICE_ALL_ACCESS, SERVICE_WIN32_OWN_PROCESS, SERVICE_AUTO_START, SERVICE_ERROR_NORMAL, fullPath, nullptr, nullptr, nullptr, nullptr, nullptr);

	if (scService == nullptr) {
		::CloseServiceHandle(scManager);
		return false;
	}

	::CloseServiceHandle(scService);
	::CloseServiceHandle(scManager);
	return true;
}

bool ServiceWin32::DeleteService() {
	SC_HANDLE scManager = ::OpenSCManagerW(nullptr, nullptr, SC_MANAGER_ALL_ACCESS);
	if (scManager == nullptr) return false;

	String name = Utf8ToSystem(serviceName);
	SC_HANDLE scService = ::OpenServiceW(scManager, (WCHAR*)name.c_str(), SERVICE_ALL_ACCESS);

	if (scService == nullptr) {
		::CloseServiceHandle(scManager);
		return false;
	}

	bool result = ::DeleteService(scService) != 0;

	::CloseServiceHandle(scService);
	::CloseServiceHandle(scManager);
	return result;
}

#endif