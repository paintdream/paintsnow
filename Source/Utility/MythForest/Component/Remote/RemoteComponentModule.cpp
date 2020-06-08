#include "RemoteComponentModule.h"
#include "../../Engine.h"

using namespace PaintsNow;
using namespace PaintsNow::NsMythForest;
using namespace PaintsNow::NsSnowyStream;

RemoteComponentModule::RemoteComponentModule(Engine& engine) : BaseClass(engine) {}
RemoteComponentModule::~RemoteComponentModule() {}

TObject<IReflect>& RemoteComponentModule::operator () (IReflect& reflect) {
	BaseClass::operator () (reflect);
	if (reflect.IsReflectMethod()) {
		ReflectMethod(RequestNew)[ScriptMethod = "New"];
		ReflectMethod(RequestRebuild)[ScriptMethod = "Rebuild"];
	}

	return *this;
}

TShared<RemoteComponent> RemoteComponentModule::RequestNew(IScript::Request& request) {
	CHECK_REFERENCES_NONE();

	TShared<RemoteComponent> remoteComponent = TShared<RemoteComponent>::From(allocator->New());
	remoteComponent->SetWarpIndex(engine.GetKernel().GetCurrentWarpIndex());
	return remoteComponent;
}

void RemoteComponentModule::RequestRebuild(IScript::Request& request, IScript::Delegate<RemoteComponent> remoteComponent) {
	CHECK_REFERENCES_NONE();
	CHECK_DELEGATE(remoteComponent);

}