#include "SurfaceComponentModule.h"

using namespace PaintsNow;

SurfaceComponentModule::SurfaceComponentModule(Engine& engine) : BaseClass(engine) {}

TObject<IReflect>& SurfaceComponentModule::operator () (IReflect& reflect) {
	BaseClass::operator () (reflect);

	if (reflect.IsReflectMethod()) {
		ReflectMethod(RequestNew)[ScriptMethod = "New"];
	}

	return *this;
}

TShared<SurfaceComponent> SurfaceComponentModule::RequestNew(IScript::Request& request) {
	CHECK_REFERENCES_NONE();

	TShared<SurfaceComponent> surfaceComponent = TShared<SurfaceComponent>::From(allocator->New());
	surfaceComponent->SetWarpIndex(engine.GetKernel().GetCurrentWarpIndex());
	return surfaceComponent;
}

