#pragma once

using namespace System;
#include "../../Source/Core/Interface/IScript.h"

namespace DotNetBridge {
	public ref class ScriptReference
	{
	private:
		size_t handle;

	public:
		ScriptReference(size_t h);
		~ScriptReference();

		property bool Valid { bool get() { return handle != 0; } };
		property size_t Handle { size_t get() { return handle; } };
		int AsInteger();
		double AsDouble();
		float AsFloat();
		System::IntPtr AsHandle();
		System::String^ AsString();
		Object^ Call(... array<Object^>^ args);
	};

	public ref class LeavesBridge
	{
	private:
		LeavesBridge();
		LeavesBridge(const LeavesBridge%);
		
		static LeavesBridge instance;

	public:
		static property LeavesBridge^ Instance { LeavesBridge^ get() { return % instance; } }
		void RegisterDelegate(System::String^ name, Delegate^ func);
		void Initialize(PaintsNow::IScript::Request& request);
		void Uninitialize(PaintsNow::IScript::Request& request);

		UIntPtr GetScriptHandle();
		ScriptReference^ GetGlobal(System::String^ name);

		ScriptReference^ moduleTable = nullptr;
		PaintsNow::IScript* script = nullptr;
		PaintsNow::IScript::RequestPool* requestPool = nullptr;
	};
}
