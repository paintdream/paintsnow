#include "RayTraceComponentModule.h"
#include "../../Engine.h"

using namespace PaintsNow;

RayTraceComponentModule::RayTraceComponentModule(Engine& engine) : BaseClass(engine) {}
RayTraceComponentModule::~RayTraceComponentModule() {}

TObject<IReflect>& RayTraceComponentModule::operator () (IReflect& reflect) {
	BaseClass::operator () (reflect);
	if (reflect.IsReflectMethod()) {
		ReflectMethod(RequestNew)[ScriptMethodLocked = "New"];
	}

	return *this;
}

TShared<RayTraceComponent> RayTraceComponentModule::RequestNew(IScript::Request& request) {
	CHECK_REFERENCES_NONE();

	TShared<RayTraceComponent> navigateComponent = TShared<RayTraceComponent>::From(allocator->New());
	navigateComponent->SetWarpIndex(engine.GetKernel().GetCurrentWarpIndex());
	return navigateComponent;
}
