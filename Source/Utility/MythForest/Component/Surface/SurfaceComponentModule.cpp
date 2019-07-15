#include "SurfaceComponentModule.h"

using namespace PaintsNow;
using namespace PaintsNow::NsMythForest;

SurfaceComponentModule::SurfaceComponentModule(Engine& engine) : BaseClass(engine) {}


TObject<IReflect>& SurfaceComponentModule::operator () (IReflect& reflect) {
	BaseClass::operator () (reflect);

	if (reflect.IsReflectMethod()) {
		ReflectMethod(RequestNew)[ScriptMethod = "New"];
		ReflectMethod(RequestRebuild)[ScriptMethod = "Rebuild"];
	}

	return *this;
}

void SurfaceComponentModule::RequestNew(IScript::Request& request) {
	CHECK_REFERENCES_NONE();

	TShared<SurfaceComponent> surfaceComponent = TShared<SurfaceComponent>::From(allocator->New());
	surfaceComponent->SetWarpIndex(engine.GetKernel().GetCurrentWarpIndex());

	engine.GetKernel().YieldCurrentWarp();
	request.DoLock();
	request << surfaceComponent;
	request.UnLock();
}

void SurfaceComponentModule::RequestRebuild(IScript::Request& request, IScript::Delegate<SurfaceComponent> surfaceComponent) {
	CHECK_REFERENCES_NONE();
	CHECK_DELEGATE(surfaceComponent);


}