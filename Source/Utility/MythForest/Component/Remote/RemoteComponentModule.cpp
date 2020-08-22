#include "RemoteComponentModule.h"
#include "../../Engine.h"

using namespace PaintsNow;

RemoteComponentModule::RemoteComponentModule(Engine& engine) : BaseClass(engine) {}
RemoteComponentModule::~RemoteComponentModule() {}

TObject<IReflect>& RemoteComponentModule::operator () (IReflect& reflect) {
	BaseClass::operator () (reflect);
	if (reflect.IsReflectMethod()) {
		ReflectMethod(RequestNew)[ScriptMethod = "New"];
		ReflectMethod(RequestLoad)[ScriptMethod = "Load"];
		ReflectMethod(RequestCall)[ScriptMethod = "Call"];
		ReflectMethod(RequestCallAsync)[ScriptMethod = "CallAsync"];
		ReflectMethod(RequestCleanup)[ScriptMethod = "Cleanup"];
	}

	return *this;
}

TShared<RemoteComponent> RemoteComponentModule::RequestNew(IScript::Request& request, bool transparentMode) {
	CHECK_REFERENCES_NONE();

	TShared<RemoteComponent> remoteComponent = TShared<RemoteComponent>::From(allocator->New(std::ref(engine)));

	if (transparentMode) {
		remoteComponent->Flag().fetch_or(RemoteComponent::REMOTECOMPONENT_TRANSPARENT, std::memory_order_acquire);
	}

	remoteComponent->SetWarpIndex(engine.GetKernel().GetCurrentWarpIndex());
	return remoteComponent;
}

TShared<RemoteRoutine> RemoteComponentModule::RequestLoad(IScript::Request& request, IScript::Delegate<RemoteComponent> remoteComponent, const String& code) {
	CHECK_REFERENCES_NONE();
	CHECK_DELEGATE(remoteComponent);
	CHECK_THREAD_IN_MODULE(remoteComponent);

	return remoteComponent->Load(code);
}

void RemoteComponentModule::RequestCall(IScript::Request& request, IScript::Delegate<RemoteComponent> remoteComponent, IScript::Delegate<RemoteRoutine> remoteRoutine, IScript::Request::Arguments& args) {
	CHECK_REFERENCES_NONE();
	CHECK_DELEGATE(remoteComponent);
	CHECK_DELEGATE(remoteRoutine);
	CHECK_THREAD_IN_MODULE(remoteComponent);

	remoteComponent->Call(request, remoteRoutine.Get(), args);
}

void RemoteComponentModule::RequestCallAsync(IScript::Request& request, IScript::Delegate<RemoteComponent> remoteComponent, IScript::Request::Ref callback, IScript::Delegate<RemoteRoutine> remoteRoutine, IScript::Request::Arguments& args) {
	CHECK_REFERENCES_NONE();
	CHECK_DELEGATE(remoteComponent);
	CHECK_DELEGATE(remoteRoutine);
	CHECK_THREAD_IN_MODULE(remoteComponent);

	remoteComponent->CallAsync(request, callback, remoteRoutine.Get(), args);
}

void RemoteComponentModule::RequestCleanup(IScript::Request& request, IScript::Delegate<RemoteComponent> remoteComponent) {
	CHECK_REFERENCES_NONE();
	CHECK_DELEGATE(remoteComponent);
	CHECK_THREAD_IN_MODULE(remoteComponent);

	remoteComponent->Clear();
}