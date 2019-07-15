#include "ProfileComponentModule.h"

using namespace PaintsNow;
using namespace PaintsNow::NsMythForest;

ProfileComponentModule::ProfileComponentModule(Engine& engine) : BaseClass(engine) {}

TObject<IReflect>& ProfileComponentModule::operator () (IReflect& reflect) {
	BaseClass::operator () (reflect);

	if (reflect.IsReflectMethod()) {
		ReflectMethod(RequestNew)[ScriptMethod = "New"];
		ReflectMethod(RequestGetInterval)[ScriptMethod = "GetInterval"];
	}

	return *this;
}

void ProfileComponentModule::RequestNew(IScript::Request& request, float historyRatio) {
	TShared<ProfileComponent> profileComponent = TShared<ProfileComponent>::From(allocator->New(historyRatio));
	profileComponent->SetWarpIndex(engine.GetKernel().GetCurrentWarpIndex());

	engine.GetKernel().YieldCurrentWarp();

	request.DoLock();
	request << profileComponent;
	request.UnLock();
}

void ProfileComponentModule::RequestGetInterval(IScript::Request& request, IScript::Delegate<ProfileComponent> profileComponent) {
	CHECK_REFERENCES_NONE();
	CHECK_DELEGATE(profileComponent);
	CHECK_THREAD_IN_MODULE(profileComponent);

	engine.GetKernel().YieldCurrentWarp();

	request.DoLock();
	request << profileComponent->GetTickInterval();
	request.UnLock();
}