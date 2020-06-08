#include "VisibilityComponentModule.h"

using namespace PaintsNow;
using namespace PaintsNow::NsMythForest;

VisibilityComponentModule::VisibilityComponentModule(Engine& engine) : BaseClass(engine) {}

TObject<IReflect>& VisibilityComponentModule::operator () (IReflect& reflect) {
	BaseClass::operator () (reflect);

	if (reflect.IsReflectMethod()) {
		ReflectMethod(RequestNew)[ScriptMethod = "New"];
		ReflectMethod(RequestSetup)[ScriptMethod = "Setup"];
	}

	return *this;
}

TShared<VisibilityComponent> VisibilityComponentModule::RequestNew(IScript::Request& request) {
	CHECK_REFERENCES_NONE();

	TShared<VisibilityComponent> visibilityComponent = TShared<VisibilityComponent>::From(allocator->New());
	visibilityComponent->SetWarpIndex(engine.GetKernel().GetCurrentWarpIndex());
	return visibilityComponent;
}

void VisibilityComponentModule::RequestSetup(IScript::Request& request, IScript::Delegate<VisibilityComponent> visibilityComponent, float maxDistance, const Float3Pair& range, const UShort3& division, uint32_t frameTimeLimit, uint32_t taskCount, const UShort2& resolution) {
	CHECK_REFERENCES_NONE();
	CHECK_DELEGATE(visibilityComponent);
	CHECK_THREAD_IN_MODULE(visibilityComponent);

	visibilityComponent->Setup(engine, maxDistance, range, division, frameTimeLimit, taskCount, resolution);
}
