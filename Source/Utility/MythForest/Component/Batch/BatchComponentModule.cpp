#include "BatchComponentModule.h"
#include "BatchComponent.h"

using namespace PaintsNow;
using namespace PaintsNow::NsMythForest;

BatchComponentModule::BatchComponentModule(Engine& engine) : BaseClass(engine) {}

TObject<IReflect>& BatchComponentModule::operator () (IReflect& reflect) {
	BaseClass::operator () (reflect);

	if (reflect.IsReflectMethod()) {
		ReflectMethod(RequestNew)[ScriptMethod = "New"];
		ReflectMethod(RequestGetCaptureStatistics)[ScriptMethod = "GetCaptureStatistics"];
	}

	return *this;
}

TShared<BatchComponent> BatchComponentModule::Create() {
	TShared<BatchComponent> batchComponent = TShared<BatchComponent>::From(allocator->New());
	batchComponent->SetWarpIndex(engine.GetKernel().GetCurrentWarpIndex());

	return batchComponent;
}

TShared<BatchComponent> BatchComponentModule::RequestNew(IScript::Request& request) {
	CHECK_REFERENCES_NONE();
	return Create();
}

void BatchComponentModule::RequestGetCaptureStatistics(IScript::Request& request, IScript::Delegate<BatchComponent> component) {
	CHECK_REFERENCES_NONE();
	CHECK_DELEGATE(component);
}