// ToolkitWin32.h
// PaintDream (paintdream@paintdream.com)
// 2021-3-1
//

#pragma once

#ifdef _WIN32

#include "../../../../Core/Interface/IScript.h"

namespace PaintsNow {
	class ToolkitWin32 : public TReflected<ToolkitWin32, IScript::Library> {
	public:
		ToolkitWin32();
		TObject<IReflect>& operator () (IReflect& reflect) override;
		~ToolkitWin32() override;

	protected:
		/// <summary>
		/// Create win32 process, returning handle
		/// </summary>
		/// <param name="path"> executable path </param>
		/// <param name="currentPath"> current folder path </param>
		/// <param name="parameter"> optional configs </param>
		/// <returns> win32 process handle </returns>
		uint64_t RequestCreateProcess(IScript::Request& request, const String& path, const String& currentPath, const String& parameter);

		/// <summary>
		/// Close win32 handle
		/// </summary>
		/// <param name="object"> object handle </param>
		void RequestCloseHandle(IScript::Request& request, uint64_t object);

		/// <summary>
		/// Wait for single object (process/thread/event)
		/// </summary>
		/// <param name="object"> object handle </param>
		/// <param name="timeout"> timeout </param>
		/// <returns> the return value of WaitForSingleObject </returns>
		uint32_t RequestWaitForSingleObject(IScript::Request& request, uint64_t object, uint64_t timeout);

		/// <summary>
		/// Terminate a process
		/// </summary>
		/// <param name="process"> process handle </param>
		/// <param name="exitCode"> exit code </param>
		/// <returns> true if successfully terminated </returns>
		bool RequestTerminateProcess(IScript::Request& request, uint64_t process, uint32_t exitCode);

		/// <summary>
		/// Load win32 library
		/// </summary>
		/// <param name="library"> library name </param>
		/// <returns> library handle </returns>
		uint64_t RequestLoadLibrary(IScript::Request& request, const String& library);

		/// <summary>
		/// Free win32 library
		/// </summary>
		/// <param name="handle"> library handle </param>
		/// <returns> true if successfully free </returns>
		bool RequestFreeLibrary(IScript::Request& request, uint64_t handle);
	};
}

#endif
