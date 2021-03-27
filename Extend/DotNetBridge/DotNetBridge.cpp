#include "DotNetBridge.h"
#include "../../Source/Core/Interface/IScript.h"

using namespace PaintsNow;
using namespace DotNetBridge;

extern "C" __declspec(dllexport) uint64_t Main(void*, IScript::Request&, void*, const char*, uint64_t, uint64_t)
{
	return 0;
}

System::String^ LeavesBridge::GetVersionInfo()
{
	return gcnew System::String("DotNetBridge");
}
