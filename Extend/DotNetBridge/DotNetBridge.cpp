#include "DotNetBridge.h"
#include <vcclr.h>

using namespace System::Text;
using namespace System::Diagnostics;
using namespace PaintsNow;
using namespace DotNetBridge;

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

static System::String^ ToManagedString(const char* str)
{
	return gcnew System::String(str, 0, (int)strlen(str), Encoding::UTF8);
}

// Serializer Helpers
static void WriteValueArray(IScript::Request& request, array<Object^>^ args);
static void WriteValue(IScript::Request& request, Object^ object)
{
	Type^ type = object->GetType();
	if (type->IsArray)
	{
		array<Object^>^ arr = dynamic_cast<array<Object^>^>(object);
		if (arr)
		{
			request << beginarray;
			WriteValueArray(request, arr);
			request << endarray;
		}
	}
}

static void WriteValueArray(IScript::Request& request, array<Object^>^ args)
{
	int lowerBound = args->GetLowerBound(0);
	int upperBound = args->GetUpperBound(0);

	if (upperBound != -1)
	{
		for (int i = lowerBound; i <= upperBound; i++)
		{
			Object^ object = args[i];
			WriteValue(request, object);
		}
	}
}

static Object^ ReadValue(IScript::Request& request)
{
	switch (request.GetCurrentType())
	{
	case IScript::Request::NIL:
	{
		void* v;
		request >> v;
		return nullptr;
	}
	case IScript::Request::BOOLEAN:
	{
		bool v = false;
		request >> v;
		return Boolean(v);
	}
	case IScript::Request::NUMBER:
	{
		double v = 0;
		request >> v;
		return Double(v);
	}
	case IScript::Request::INTEGER:
	{
		int v = 0;
		request >> v;
		return v;
	}
	case IScript::Request::STRING:
	{
		const char* s = nullptr;
		request >> s;
		return ToManagedString(s);
	}
	case IScript::Request::TABLE:
	case IScript::Request::ARRAY:
	{
		// TODO: support dict
		IScript::Request::TableStart ts;
		request >> ts;
		array<Object^>^ arr = gcnew array<Object^>((int)ts.count);
		for (int i = 0; i < ts.count; i++)
		{
			arr[i] = ReadValue(request);
		}
		request >> endtable;
		return arr;
	}
	case IScript::Request::FUNCTION:
	case IScript::Request::OBJECT:
	{
		IScript::Request::Ref r;
		request >> r;
		if (r)
		{
			return gcnew ScriptReference(r.value);
		}
		else
		{
			return nullptr;
		}
	}
	}

	return nullptr;
}

class SharpReference : public IScript::Object
{
public:
	SharpReference(Delegate^ d) : function(d) {}
	void RequestCall(IScript::Request& request, IScript::Request::Arguments args)
	{
		Delegate^ d = function;
		if (d)
		{
			d->DynamicInvoke();
		}
	}

private:
	gcroot<Delegate^> function;
};

ScriptReference::ScriptReference(size_t h) : handle(h) {}
ScriptReference::~ScriptReference()
{
	IScript* script = LeavesBridge::Instance->script;
	script->DoLock();
	script->GetDefaultRequest().Dereference(IScript::Request::Ref(handle));
	script->UnLock();
}

Object^ ScriptReference::Call(... array<Object^>^ args)
{
	if (!Valid)
	{
		throw gcnew NullReferenceException("Unable to call invalid function.");
	}

	IScript::RequestPool* requestPool = LeavesBridge::Instance->requestPool;
	IScript::Request& request = *requestPool->requestPool.AcquireSafe();
	IScript::Request::Ref f(handle);

	request.DoLock();
	request.Push();
	WriteValueArray(request, args);
	request.Call(f);
	Object^ retValue = ReadValue(request);
	request.Pop();
	request.UnLock();

	requestPool->requestPool.ReleaseSafe(&request);
	return retValue;
}

template <typename T>
static T AsObject(size_t handle)
{
	if (handle == 0) return T();

	T t;
	IScript::Request& request = LeavesBridge::Instance->script->GetDefaultRequest();
	request.DoLock();
	request.Push();
	request << IScript::Request::Ref(handle);
	request >> t;
	request.Pop();
	request.UnLock();

	return t;
}

int ScriptReference::AsInteger()
{
	return AsObject<int>(handle);
}

double ScriptReference::AsDouble()
{
	return AsObject<double>(handle);
}

float ScriptReference::AsFloat()
{
	return AsObject<float>(handle);
}

System::IntPtr ScriptReference::AsHandle()
{
	return System::IntPtr(AsObject<void*>(handle));
}

System::String^ ScriptReference::AsString()
{
	return ToManagedString(AsObject<const char*>(handle));
}

UIntPtr LeavesBridge::GetScriptHandle()
{
	return UIntPtr(script);
}

ScriptReference^ LeavesBridge::GetGlobal(System::String^ name)
{
	Debug::Assert(script != nullptr);

	IScript::Request& request = script->GetDefaultRequest();
	IScript::Request::Ref r;
	request.DoLock();
	request << global >> key(FromManagedString(name).c_str()) >> r << endtable;
	request.UnLock();

	return gcnew ScriptReference(r.value);
}

void LeavesBridge::Initialize(IScript::Request& request)
{
	Debug::Assert(script == nullptr);

	script = request.GetScript();
	requestPool = request.GetRequestPool();

	IScript::Request::Ref r;
	request.DoLock();
	request.Push();
	request << begintable;
	request << endtable;
	request >> r;
	request.Pop();
	request << r;
	request.UnLock();

	moduleTable = gcnew ScriptReference(r.value);
}

void LeavesBridge::Uninitialize(IScript::Request& request)
{
	Debug::Assert(script != nullptr);

	script = nullptr;
	requestPool = nullptr;
	moduleTable = nullptr;
}

void LeavesBridge::RegisterDelegate(System::String^ name, Delegate^ func)
{
	SharpReference* sharpRef = new SharpReference(func);
	IScript::Request& request = LeavesBridge::Instance->script->GetDefaultRequest();

	PaintsNow::String funcName = FromManagedString(name);
	request.DoLock();
	request.Push();
	request << IScript::Request::Ref(moduleTable->Handle);
	request >> begintable;
	request << key(funcName) << request.Adapt(Wrap(sharpRef, &SharpReference::RequestCall));
	request >> key("__delegate__"); // in case of destroy!!
		request >> begintable;
		request << key(funcName) << sharpRef;
		request >> endtable;
	request >> endtable;
	request.Pop();
	request.UnLock();
}

LeavesBridge::LeavesBridge() {}
LeavesBridge::LeavesBridge(const LeavesBridge%)
{
	throw gcnew InvalidOperationException("LeavesBridge cannot copy");
}

extern "C" __declspec(dllexport) size_t Main(void*, IScript::Request& request, void*, const char* option, size_t, size_t)
{
	std::string strOption = option;
	LeavesBridge^ bridge = LeavesBridge::Instance;

	if (strOption == "Initialize")
	{
		bridge->Initialize(request);
	}
	else if (strOption == "Uninitialize")
	{
		bridge->Uninitialize(request);
	}

	return 0;
}
