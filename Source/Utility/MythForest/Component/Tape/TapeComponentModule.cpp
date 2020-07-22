#include "TapeComponentModule.h"
#include "TapeComponent.h"

using namespace PaintsNow;
using namespace PaintsNow::NsMythForest;

TapeComponentModule::TapeComponentModule(Engine& engine) : BaseClass(engine) {}

TObject<IReflect>& TapeComponentModule::operator () (IReflect& reflect) {
	BaseClass::operator () (reflect);

	if (reflect.IsReflectMethod()) {
		ReflectMethod(RequestNew)[ScriptMethod = "New"];
	}

	return *this;
}

TShared<TapeComponent> TapeComponentModule::RequestNew(IScript::Request& request) {
	CHECK_REFERENCES_NONE();

	TShared<TapeComponent> tapeComponent = TShared<TapeComponent>::From(allocator->New());
	tapeComponent->SetWarpIndex(engine.GetKernel().GetCurrentWarpIndex());
	return tapeComponent;
}
