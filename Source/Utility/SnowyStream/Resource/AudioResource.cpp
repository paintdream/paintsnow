#include "AudioResource.h"

using namespace PaintsNow;
using namespace PaintsNow::NsSnowyStream;

AudioResource::AudioResource(ResourceManager& manager, const ResourceManager::UniqueLocation& uniqueID) : BaseClass(manager, uniqueID), audioBuffer(nullptr) {
	Flag() |= RESOURCE_STREAM;
}

void AudioResource::Download(IAudio& audio, void* deviceContext) {
}

void AudioResource::Upload(IAudio& audio, void* deviceContext) {
	// no operations ...
}

void AudioResource::Attach(IAudio& audio, void* deviceContext) {
	audioBuffer = audio.CreateBuffer();
}

void AudioResource::Detach(IAudio& audio, void* deviceContext) {
	audio.DeleteBuffer(audioBuffer);
}

IAudio::Buffer* AudioResource::GetAudioBuffer() {
	return audioBuffer;
}

TObject<IReflect>& AudioResource::operator () (IReflect& reflect) {
	BaseClass::operator () (reflect);

	if (reflect.IsReflectProperty()) {
		ReflectProperty(audioBuffer)[Runtime];
	}

	return *this;
}

bool AudioResource::operator << (IStreamBase& stream) {
	if (!(BaseClass::operator << (stream))) {
		return false;
	}

	// ignore payload, treat as online stream!
	IStreamBase& baseStream = stream.GetBaseStream();
	localStream << baseStream;
	return true;
}

bool AudioResource::operator >> (IStreamBase& stream) const {
	if (!(BaseClass::operator >> (stream))) {
		return false;
	}

	IStreamBase& baseStream = stream.GetBaseStream();
	return localStream >> baseStream;
}

ZLocalStream& AudioResource::GetLocalStream() {
	return localStream;
}

IReflectObject* AudioResource::Clone() const {
	AudioResource* clone = new AudioResource(resourceManager, "");
	assert(audioBuffer == nullptr); // must not attach
	clone->localStream = localStream;
	// TODO:
	return clone;
}
