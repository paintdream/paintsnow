#pragma once

using namespace System;
#include "../../Source/Core/Interface/IScript.h"

namespace DotNetBridge {
	public ref class ScriptReference
	{
	public:
		ScriptReference(size_t h);
		~ScriptReference();

	private:
		size_t handle;
	};

	public ref class LeavesBridge
	{
	public:
		UIntPtr GetScriptHandle();
		ScriptReference^ GetGlobal(System::String^ name);
	};
}
