#include "SoundComponent.h"

using namespace PaintsNow;
using namespace PaintsNow::NsMythForest;
using namespace PaintsNow::NsSnowyStream;

SoundComponent::SoundComponent(TShared<AudioResource> resource, IScript::Request::Ref r) : callback(r), audioSource(nullptr), audioBuffer(nullptr) {
	Flag() |= SOUNDCOMPONENT_ONLINE;

	audioResource.Reset(static_cast<AudioResource*>(resource->Clone()));
}

void SoundComponent::Initialize(Engine& engine, Entity* entity) {
	IAudio& audio = engine.interfaces.audio;

	audioBuffer = audio.CreateBuffer();
	audioSource = audio.CreateSource();
	audio.SetBufferStream(audioBuffer, *audioResource->GetAudioStream(), IsOnline());
	stepWrapper = audio.SetSourceBuffer(audioSource, audioBuffer);
}

void SoundComponent::Uninitialize(Engine& engine, Entity* entity) {
	IAudio& audio = engine.interfaces.audio;
	audio.DeleteSource(audioSource);
	audio.DeleteBuffer(audioBuffer);
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
	engine.interfaces.audio.Play(audioSource);
}

void SoundComponent::Pause(Engine& engine) {
	Flag() &= ~TINY_ACTIVATED;
	engine.interfaces.audio.Pause(audioSource);
}

void SoundComponent::Stop(Engine& engine) {
	Flag() &= ~TINY_ACTIVATED;
	engine.interfaces.audio.Stop(audioSource);
}

void SoundComponent::Seek(Engine& engine, double time) {
	IAudio::Decoder* audioStream = audioResource->GetAudioStream();
	audioStream->Seek(IStreamBase::BEGIN, (long)(time * audioStream->GetSampleRate()));
	// engine.interfaces.audio.Seek(audioSource, IStreamBase::CUR, time);
}

void SoundComponent::Rewind(Engine& engine) {
	engine.interfaces.audio.Rewind(audioSource);
}
