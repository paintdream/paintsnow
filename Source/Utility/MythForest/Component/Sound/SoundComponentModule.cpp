#include "SoundComponentModule.h"
#include "../../Engine.h"

using namespace PaintsNow;
using namespace PaintsNow::NsMythForest;
using namespace PaintsNow::NsSnowyStream;

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

void SoundComponentModule::RequestNew(IScript::Request& request, String path, IScript::Request::Ref callback) {
	CHECK_REFERENCES_WITH_TYPE(callback, IScript::Request::FUNCTION);

	TShared<AudioResource> audioResource = engine.snowyStream.CreateReflectedResource(UniqueType<AudioResource>(), path);
	if (audioResource) {
		TShared<SoundComponent> soundComponent = TShared<SoundComponent>::From(allocator->New(audioResource, callback));
		soundComponent->SetWarpIndex(engine.GetKernel().GetCurrentWarpIndex());
		engine.GetKernel().YieldCurrentWarp();

		request.DoLock();
		request << soundComponent;
		request.UnLock();
	} else {
		request.Error(String("SoundComponentModule::RequestNewSource: invalid path '") + path + "'");
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

void SoundComponentModule::RequestGetSourceDuration(IScript::Request& request, IScript::Delegate<SoundComponent> source) {
	CHECK_REFERENCES_NONE();
	CHECK_DELEGATE(source);
	int64_t duration = source->GetDuration();
	engine.GetKernel().YieldCurrentWarp();

	request.DoLock();
	request << duration;
	request.UnLock();
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

void SoundComponentModule::RequestIsSourcePaused(IScript::Request& request, IScript::Delegate<SoundComponent> source) {
	assert(false);
}
