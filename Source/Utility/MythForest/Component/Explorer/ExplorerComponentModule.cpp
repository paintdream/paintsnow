#include "ExplorerComponentModule.h"
#include "../../Engine.h"

using namespace PaintsNow;
using namespace PaintsNow::NsMythForest;
using namespace PaintsNow::NsSnowyStream;

ExplorerComponentModule::ExplorerComponentModule(Engine& engine) : BaseClass(engine) {}
ExplorerComponentModule::~ExplorerComponentModule() {}

TObject<IReflect>& ExplorerComponentModule::operator () (IReflect& reflect) {
	BaseClass::operator () (reflect);
	if (reflect.IsReflectMethod()) {
		ReflectMethod(RequestNew)[ScriptMethod = "New"];
		ReflectMethod(RequestRebuild)[ScriptMethod = "Rebuild"];
	}

	return *this;
}
void ExplorerComponentModule::RequestNew(IScript::Request& request) {
	CHECK_REFERENCES_NONE();

	TShared<ExplorerComponent> fieldComponent = TShared<ExplorerComponent>::From(allocator->New());
	fieldComponent->SetWarpIndex(engine.GetKernel().GetCurrentWarpIndex());
	engine.GetKernel().YieldCurrentWarp();
	request.DoLock();
	request << fieldComponent;
	request.UnLock();
}

void ExplorerComponentModule::RequestRebuild(IScript::Request& request, IScript::Delegate<ExplorerComponent> fieldComponent) {
	CHECK_REFERENCES_NONE();
	CHECK_DELEGATE(fieldComponent);

}