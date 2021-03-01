// ServiceWin32.h
// PaintDream (paintdream@paintdream.com)
// 2021-2-28
//

#pragma once

#ifdef _WIN32

#include "../../../LeavesFlute/LeavesFlute.h"
#include "ToolkitWin32.h"
#include <Windows.h>

namespace PaintsNow {
	class ServiceWin32 {
	public:
		ServiceWin32(const String& serviceName);
		void ServiceMain(DWORD argc, LPTSTR* argv);
		void ServiceCtrlHandler(DWORD opcode);
		bool InstallService();
		bool DeleteService();
		static ServiceWin32& GetInstance();
		static bool InServiceManager();
		bool RunService();

	protected:
		void ConsoleHandler(LeavesFlute& leavesFlute);

	protected:
		String serviceName;
		SERVICE_STATUS serviceStatus;
		SERVICE_STATUS_HANDLE serviceStatusHandle;
		HANDLE eventStop;
		ToolkitWin32 toolkitWin32;
	};
}

#endif