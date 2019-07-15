#include "NavigateComponentModule.h"
#include "../../Engine.h"

using namespace PaintsNow;
using namespace PaintsNow::NsMythForest;
using namespace PaintsNow::NsSnowyStream;

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
void NavigateComponentModule::RequestNew(IScript::Request& request) {
	CHECK_REFERENCES_NONE();

	TShared<NavigateComponent> fieldComponent = TShared<NavigateComponent>::From(allocator->New());
	fieldComponent->SetWarpIndex(engine.GetKernel().GetCurrentWarpIndex());

	engine.GetKernel().YieldCurrentWarp();
	request.DoLock();
	request << fieldComponent;
	request.UnLock();
}

void NavigateComponentModule::RequestRebuild(IScript::Request& request, IScript::Delegate<NavigateComponent> fieldComponent) {
	CHECK_REFERENCES_NONE();
	CHECK_DELEGATE(fieldComponent);

}