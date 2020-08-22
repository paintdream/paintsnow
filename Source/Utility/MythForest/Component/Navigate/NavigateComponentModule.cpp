#include "NavigateComponentModule.h"
#include "../../Engine.h"

using namespace PaintsNow;

NavigateComponentModule::NavigateComponentModule(Engine& engine) : BaseClass(engine) {}
NavigateComponentModule::~NavigateComponentModule() {}

TObject<IReflect>& NavigateComponentModule::operator () (IReflect& reflect) {
	BaseClass::operator () (reflect);
	if (reflect.IsReflectMethod()) {
		ReflectMethod(RequestNew)[ScriptMethod = "New"];
		ReflectMethod(RequestRebuild)[ScriptMethod = "Rebuild"];
	}

	return *this;
}

TShared<NavigateComponent> NavigateComponentModule::RequestNew(IScript::Request& request) {
	CHECK_REFERENCES_NONE();

	TShared<NavigateComponent> navigateComponent = TShared<NavigateComponent>::From(allocator->New());
	navigateComponent->SetWarpIndex(engine.GetKernel().GetCurrentWarpIndex());
	return navigateComponent;
}

void NavigateComponentModule::RequestRebuild(IScript::Request& request, IScript::Delegate<NavigateComponent> navigateComponent) {
	CHECK_REFERENCES_NONE();
	CHECK_DELEGATE(navigateComponent);

}