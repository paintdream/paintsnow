#include "DotNetBridge.h"

using namespace System::Text;
using namespace System::Diagnostics;
using namespace PaintsNow;
using namespace DotNetBridge;

static IScript* theScript = nullptr;

ScriptReference::ScriptReference(size_t h) : handle(h) {}
ScriptReference::~ScriptReference()
{
	IScript* script = theScript;
	script->DoLock();
	script->GetDefaultRequest().Dereference(IScript::Request::Ref(handle));
	script->UnLock();
}

UIntPtr LeavesBridge::GetScriptHandle()
{
	return UIntPtr(theScript);
}

static PaintsNow::String FromManagedString(System::String^ str)
{
	array<Byte>^ byteArray = Encoding::UTF8->GetBytes(str);
	pin_ptr<unsigned char> v = &byteArray[0];
	return PaintsNow::String(reinterpret_cast<char*>(v), byteArray->Length);
}

static System::String^ ToManagedString(const PaintsNow::String& str)
{
	return gcnew System::String(str.c_str(), 0, (int)str.length(), Encoding::UTF8);
}

ScriptReference^ LeavesBridge::GetGlobal(System::String^ name)
{
	Debug::Assert(theScript != nullptr);

	IScript::Request& request = theScript->GetDefaultRequest();
	IScript::Request::Ref r;
	request.DoLock();
	request << global >> key(FromManagedString(name).c_str()) >> r << endtable;
	request.UnLock();

	return gcnew ScriptReference(r.value);
}


extern "C" __declspec(dllexport) size_t Main(void*, IScript::Request& request, void*, const char* option, size_t, size_t)
{
	std::string strOption = option;

	if (strOption == "Initialize")
	{
		theScript = request.GetScript();
	}
	else if (strOption == "Uninitialize")
	{
		theScript = nullptr;
	}

	return 0;
}
