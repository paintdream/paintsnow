#include "NavigateComponentModule.h"
#include "../../Engine.h"

using namespace PaintsNow;

NavigateComponentModule::NavigateComponentModule(Engine& engine) : BaseClass(engine) {}
NavigateComponentModule::~NavigateComponentModule() {}

TObject<IReflect>& NavigateComponentModule::operator () (IReflect& reflect) {
	BaseClass::operator () (reflect);
	if (reflect.IsReflectMethod()) {
		ReflectMethod(RequestNew)[ScriptMethod = "New"];
	}

	return *this;
}

TShared<NavigateComponent> NavigateComponentModule::RequestNew(IScript::Request& request) {
	CHECK_REFERENCES_NONE();

	TShared<NavigateComponent> navigateComponent = TShared<NavigateComponent>::From(allocator->New());
	navigateComponent->SetWarpIndex(engine.GetKernel().GetCurrentWarpIndex());
	return navigateComponent;
}
