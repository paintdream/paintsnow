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
		ReflectMethod(RequestCallAsync)[ScriptMethod = "CallAsync"];
		ReflectMethod(RequestCleanup)[ScriptMethod = "Cleanup"];
	}

	return *this;
}

TShared<ComputeComponent> ComputeComponentModule::RequestNew(IScript::Request& request, bool transparentMode) {
	CHECK_REFERENCES_NONE();

	TShared<ComputeComponent> computeComponent = TShared<ComputeComponent>::From(allocator->New(std::ref(engine)));

	if (transparentMode) {
		computeComponent->Flag().fetch_or(ComputeComponent::COMPUTECOMPONENT_TRANSPARENT, std::memory_order_acquire);
	}

	computeComponent->SetWarpIndex(engine.GetKernel().GetCurrentWarpIndex());
	return computeComponent;
}

TShared<ComputeRoutine> ComputeComponentModule::RequestLoad(IScript::Request& request, IScript::Delegate<ComputeComponent> computeComponent, const String& code) {
	CHECK_REFERENCES_NONE();
	CHECK_DELEGATE(computeComponent);
	CHECK_THREAD_IN_MODULE(computeComponent);

	return computeComponent->Load(code);
}

void ComputeComponentModule::RequestCall(IScript::Request& request, IScript::Delegate<ComputeComponent> computeComponent, IScript::Delegate<ComputeRoutine> computeRoutine, IScript::Request::Arguments& args) {
	CHECK_REFERENCES_NONE();
	CHECK_DELEGATE(computeComponent);
	CHECK_DELEGATE(computeRoutine);
	CHECK_THREAD_IN_MODULE(computeComponent);

	computeComponent->Call(request, computeRoutine.Get(), args);
}

void ComputeComponentModule::RequestCallAsync(IScript::Request& request, IScript::Delegate<ComputeComponent> computeComponent, IScript::Request::Ref callback, IScript::Delegate<ComputeRoutine> computeRoutine, IScript::Request::Arguments& args) {
	CHECK_REFERENCES_NONE();
	CHECK_DELEGATE(computeComponent);
	CHECK_DELEGATE(computeRoutine);
	CHECK_THREAD_IN_MODULE(computeComponent);

	computeComponent->CallAsync(request, callback, computeRoutine.Get(), args);
}

void ComputeComponentModule::RequestCleanup(IScript::Request& request, IScript::Delegate<ComputeComponent> computeComponent) {
	CHECK_REFERENCES_NONE();
	CHECK_DELEGATE(computeComponent);
	CHECK_THREAD_IN_MODULE(computeComponent);

	computeComponent->Clear();
}