#include "RasterizeComponentModule.h"

using namespace PaintsNow;

RasterizeComponentModule::RasterizeComponentModule(Engine& engine) : BaseClass(engine) {}

TObject<IReflect>& RasterizeComponentModule::operator () (IReflect& reflect) {
	BaseClass::operator () (reflect);

	if (reflect.IsReflectMethod()) {
		ReflectMethod(RequestNew)[ScriptMethodLocked = "New"];
	}

	return *this;
}

TShared<RasterizeComponent> RasterizeComponentModule::RequestNew(IScript::Request& request) {
	TShared<RasterizeComponent> profileComponent = TShared<RasterizeComponent>::From(allocator->New());
	profileComponent->SetWarpIndex(engine.GetKernel().GetCurrentWarpIndex());
	return profileComponent;
}
