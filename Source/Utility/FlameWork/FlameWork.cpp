#include "FlameWork.h"

using namespace PaintsNow;
using namespace PaintsNow::NsFlameWork;
using namespace PaintsNow::NsBridgeSunset;

FlameWork::FlameWork(IThread& threadApi, IScript& ns, BridgeSunset& bs) : nativeScript(ns), bridgeSunset(bs) {}

TShared<Native> FlameWork::RequestCompileNativeCode(IScript::Request& request, const String& code) {
	TShared<Native> native = TShared<Native>::From(new Native(nativeScript));
	if (native->Compile(code)) {
		native->SetWarpIndex(bridgeSunset.GetKernel().GetCurrentWarpIndex());
		bridgeSunset.GetKernel().YieldCurrentWarp();

		return native;
	} else {
		return nullptr;
	}
}

void FlameWork::RequestExecuteNative(IScript::Request& request, IScript::Delegate<Native> native, const String& entry, std::vector<String>& params) {
	CHECK_REFERENCES_NONE();
	CHECK_DELEGATE(native);
	CHECK_THREAD_IN_LIBRARY(native);
	
	native->Execute(request, entry, params);
}

TObject<IReflect>& FlameWork::operator () (IReflect& reflect) {
	BaseClass::operator () (reflect);
	if (reflect.IsReflectProperty()) {
		ReflectMethod(RequestCompileNativeCode)[ScriptMethod = "CompileNativeCode"];
		ReflectMethod(RequestExecuteNative)[ScriptMethod = "ExecuteNative"];
	}

	return *this;
}
