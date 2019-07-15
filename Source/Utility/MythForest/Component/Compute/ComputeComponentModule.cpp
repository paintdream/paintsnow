#include "ComputeComponentModule.h"
#include "../../Engine.h"

using namespace PaintsNow;
using namespace PaintsNow::NsMythForest;
using namespace PaintsNow::NsSnowyStream;

ComputeComponentModule::ComputeComponentModule(Engine& engine) : BaseClass(engine) {}
ComputeComponentModule::~ComputeComponentModule() {}

TObject<IReflect>& ComputeComponentModule::operator () (IReflect& reflect) {
	BaseClass::operator () (reflect);
	if (reflect.IsReflectMethod()) {
		ReflectMethod(RequestNew)[ScriptMethod = "New"];
		ReflectMethod(RequestLoad)[ScriptMethod = "Load"];
		ReflectMethod(RequestCall)[ScriptMethod = "Call"];
		ReflectMethod(RequestCleanup)[ScriptMethod = "Cleanup"];
	}

	return *this;
}

void ComputeComponentModule::RequestNew(IScript::Request& request) {
	CHECK_REFERENCES_NONE();

	IScript* script = engine.interfaces.script.NewScript();
	if (script != nullptr) {
		TShared<ComputeComponent> computeComponent = TShared<ComputeComponent>::From(allocator->New(std::ref(engine), script, *request.GetScript()));
		computeComponent->SetWarpIndex(engine.GetKernel().GetCurrentWarpIndex());
		engine.GetKernel().YieldCurrentWarp();

		request.DoLock();
		request << computeComponent;
		request.UnLock();
	}
}

void ComputeComponentModule::RequestLoad(IScript::Request& request, IScript::Delegate<ComputeComponent> computeComponent, const String& code) {
	CHECK_REFERENCES_NONE();
	CHECK_DELEGATE(computeComponent);
	CHECK_THREAD_IN_MODULE(computeComponent);

	TShared<ComputeRoutine> routine = computeComponent->Load(code);
	if (routine) {
		engine.GetKernel().YieldCurrentWarp();
		request.DoLock();
		request << routine;
		request.UnLock();
	}
}

void ComputeComponentModule::RequestCall(IScript::Request& request, IScript::Delegate<ComputeComponent> computeComponent, IScript::Delegate<ComputeRoutine> computeRoutine, IScript::Request::PlaceHolder ph) {
	CHECK_REFERENCES_NONE();
	CHECK_DELEGATE(computeComponent);
	CHECK_DELEGATE(computeRoutine);
	CHECK_THREAD_IN_MODULE(computeComponent);

	computeComponent->Call(request, computeRoutine.Get());
}

void ComputeComponentModule::RequestCleanup(IScript::Request& request, IScript::Delegate<ComputeComponent> computeComponent) {
	CHECK_REFERENCES_NONE();
	CHECK_DELEGATE(computeComponent);
	CHECK_THREAD_IN_MODULE(computeComponent);

	computeComponent->Cleanup();
}