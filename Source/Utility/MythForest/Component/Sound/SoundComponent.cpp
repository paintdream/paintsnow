#include "SoundComponent.h"

using namespace PaintsNow;
using namespace PaintsNow::NsMythForest;
using namespace PaintsNow::NsSnowyStream;

SoundComponent::SoundComponent(TShared<AudioResource> resource, IScript::Request::Ref r) : callback(r), source(nullptr), decoder(nullptr) {
	audioResource.Reset(static_cast<AudioResource*>(resource->Clone()));
	Flag() |= SOUNDCOMPONENT_ONLINE;
}

void SoundComponent::Initialize(Engine& engine, Entity* entity) {
	IAudio& audio = engine.interfaces.audio;
	IFilterBase& audioFilterBase = engine.interfaces.audioFilterBase;

	source = audio.CreateSource();
	decoder = audioFilterBase.CreateFilter(audioResource->GetLocalStream());
	assert(decoder->QueryInterface(UniqueType<IAudio::Decoder>()) != nullptr);
	stepWrapper = audio.SetSourceBuffer(source, audioResource->GetAudioBuffer());
	audio.SetBufferStream(audioResource->GetAudioBuffer(), static_cast<IAudio::Decoder&>(*decoder), IsOnline());
}

void SoundComponent::Uninitialize(Engine& engine, Entity* entity) {
	engine.interfaces.audio.DeleteSource(source);
	decoder->ReleaseObject();
	source = nullptr;
	decoder = nullptr;
}

void SoundComponent::ScriptUninitialize(IScript::Request& request) {
	request.DoLock();
	if (callback) {
		request.Dereference(callback);
	}
	request.UnLock();

	BaseClass::ScriptUninitialize(request);
}

bool SoundComponent::IsOnline() const {
	return !!(Flag() & SOUNDCOMPONENT_ONLINE);
}

int64_t SoundComponent::GetDuration() const {
	return 0;
}

TObject<IReflect>& SoundComponent::operator () (IReflect& reflect) {
	BaseClass::operator () (reflect);

	if (reflect.IsReflectMethod()) {
		ReflectMethod(GetDuration);
		ReflectMethod(Seek);
		ReflectMethod(Play);
		ReflectMethod(Pause);
		ReflectMethod(Stop);
		ReflectMethod(Rewind);
		ReflectMethod(IsOnline);
		ReflectMethod(IsPlaying);
	}

	return *this;
}

bool SoundComponent::IsPlaying() const {
	return !!(Flag() & Tiny::TINY_ACTIVATED);
}

void SoundComponent::Step(IScript::Request& request) {
	if (stepWrapper) {
		size_t t = stepWrapper();
		if (t == (size_t)-1) {
			Flag() &= ~Tiny::TINY_ACTIVATED;
		}

		if (t != 0 && callback) {
			request.DoLock();
			request.Push();
			request << (t == (size_t)-1 ? (int64_t)0 : (int64_t)t);
			request.Call(sync, callback);
			request.Pop();
			request.UnLock();
		}
	}
}

SoundComponent::~SoundComponent() {
}

void SoundComponent::Play(Engine& engine) {
	Flag() |= TINY_ACTIVATED;
	engine.interfaces.audio.Play(source);
}

void SoundComponent::Pause(Engine& engine) {
	Flag() &= ~TINY_ACTIVATED;
	engine.interfaces.audio.Pause(source);
}

void SoundComponent::Stop(Engine& engine) {
	Flag() &= ~TINY_ACTIVATED;
	engine.interfaces.audio.Stop(source);
}

void SoundComponent::Seek(Engine& engine, double time) {

	// buffer->decoder->Seek(IStreamBase::BEGIN, (long)(time * buffer->decoder->GetSampleRate()));
	// engine.interfaces.audio.Seek(source, IStreamBase::CUR, time);
}

void SoundComponent::Rewind(Engine& engine) {
	engine.interfaces.audio.Rewind(source);
}
