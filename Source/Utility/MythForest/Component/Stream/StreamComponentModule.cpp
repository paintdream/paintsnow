#include "StreamComponentModule.h"
#include "../../Engine.h"

using namespace PaintsNow;

StreamComponentModule::StreamComponentModule(Engine& engine) : BaseClass(engine) {}
StreamComponentModule::~StreamComponentModule() {}

TObject<IReflect>& StreamComponentModule::operator () (IReflect& reflect) {
	BaseClass::operator () (reflect);
	if (reflect.IsReflectMethod()) {
		ReflectMethod(RequestNew)[ScriptMethod = "New"];
		ReflectMethod(RequestSetStreamLoadHandler)[ScriptMethod = "SetStreamLoadHandler"];
		ReflectMethod(RequestSetStreamUnloadHandler)[ScriptMethod = "SetStreamUnloadHandler"];
	}

	return *this;
}

TShared<StreamComponent> StreamComponentModule::RequestNew(IScript::Request& request, const UShort3& dimension, uint16_t cacheCount) {
	TShared<StreamComponent> soundComponent = TShared<StreamComponent>::From(allocator->New(dimension, cacheCount));
	soundComponent->SetWarpIndex(engine.GetKernel().GetCurrentWarpIndex());
	return soundComponent;
}

void StreamComponentModule::RequestSetStreamLoadHandler(IScript::Request& request, IScript::Delegate<StreamComponent> stream, IScript::Request::Ref ref) {
	CHECK_REFERENCES_NONE();
	CHECK_DELEGATE(stream);

	stream->SetLoadHandler(request, ref);
}

void StreamComponentModule::RequestSetStreamUnloadHandler(IScript::Request& request, IScript::Delegate<StreamComponent> stream, IScript::Request::Ref ref) {
	CHECK_REFERENCES_NONE();
	CHECK_DELEGATE(stream);

	stream->SetUnloadHandler(request, ref);
}
