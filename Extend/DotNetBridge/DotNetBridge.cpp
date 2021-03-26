#include "DotNetBridge.h"
#include "../../Source/Core/Interface/IScript.h"

using namespace PaintsNow;
class Interfaces;

extern "C" __declspec(dllexport) uint64_t SetupProxy(void*, IScript::Request&, void*, const char*, uint64_t, uint64_t)
{
	return 0;
}
