#include "SoundComponentModule.h"
#include "../../Engine.h"

using namespace PaintsNow;

SoundComponentModule::SoundComponentModule(Engine& engine) : BaseClass(engine) {}
SoundComponentModule::~SoundComponentModule() {}

TObject<IReflect>& SoundComponentModule::operator () (IReflect& reflect) {
	BaseClass::operator () (reflect);
	if (reflect.IsReflectMethod()) {
		ReflectMethod(RequestNew)[ScriptMethod = "New"];
		ReflectMethod(RequestGetSourceDuration)[ScriptMethod = "GetSourceDuration"];
		ReflectMethod(RequestMoveSource)[ScriptMethod = "MoveSource"];
		ReflectMethod(RequestSeekSource)[ScriptMethod = "SeekSource"];
		ReflectMethod(RequestPlaySource)[ScriptMethod = "PlaySource"];
		ReflectMethod(RequestPauseSource)[ScriptMethod = "PauseSource"];
		ReflectMethod(RequestStopSource)[ScriptMethod = "StopSource"];
		ReflectMethod(RequestRewindSource)[ScriptMethod = "RewindSource"];
		ReflectMethod(RequestIsSourcePaused)[ScriptMethod = "IsSourcePaused"];
	}

	return *this;
}

TShared<SoundComponent> SoundComponentModule::RequestNew(IScript::Request& request, String path, IScript::Request::Ref callback) {
	CHECK_REFERENCES_WITH_TYPE(callback, IScript::Request::FUNCTION);

	TShared<StreamResource> audioResource = engine.snowyStream.CreateReflectedResource(UniqueType<StreamResource>(), path);
	if (audioResource) {
		TShared<SoundComponent> soundComponent = TShared<SoundComponent>::From(allocator->New(audioResource, callback));
		soundComponent->SetWarpIndex(engine.GetKernel().GetCurrentWarpIndex());
		return soundComponent;
	} else {
		request.Error(String("SoundComponentModule::RequestNewSource: invalid path '") + path + "'");
		return nullptr;
	}
}

void SoundComponentModule::RequestMoveSource(IScript::Request& request, IScript::Delegate<SoundComponent> source, Float3& pos) {
	CHECK_REFERENCES_NONE();
	CHECK_DELEGATE(source);
}

void SoundComponentModule::RequestSeekSource(IScript::Request& request, IScript::Delegate<SoundComponent> source, double time) {
	CHECK_REFERENCES_NONE();
	CHECK_DELEGATE(source);
	source->Seek(engine, time);
}

int64_t SoundComponentModule::RequestGetSourceDuration(IScript::Request& request, IScript::Delegate<SoundComponent> source) {
	CHECK_REFERENCES_NONE();
	CHECK_DELEGATE(source);
	return source->GetDuration();
}

void SoundComponentModule::RequestPlaySource(IScript::Request& request, IScript::Delegate<SoundComponent> source) {
	CHECK_REFERENCES_NONE();
	CHECK_DELEGATE(source);
	source->Play(engine);
}

void SoundComponentModule::RequestPauseSource(IScript::Request& request, IScript::Delegate<SoundComponent> source) {
	CHECK_REFERENCES_NONE();
	CHECK_DELEGATE(source);
	source->Pause(engine);
}

void SoundComponentModule::RequestStopSource(IScript::Request& request, IScript::Delegate<SoundComponent> source) {
	CHECK_REFERENCES_NONE();
	CHECK_DELEGATE(source);
	source->Stop(engine);
}

void SoundComponentModule::RequestRewindSource(IScript::Request& request, IScript::Delegate<SoundComponent> source) {
	CHECK_REFERENCES_NONE();
	CHECK_DELEGATE(source);
	source->Rewind(engine);
}

bool SoundComponentModule::RequestIsSourcePaused(IScript::Request& request, IScript::Delegate<SoundComponent> source) {
	return !source->IsPlaying();
}
