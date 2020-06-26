#include "AudioResource.h"

using namespace PaintsNow;
using namespace PaintsNow::NsSnowyStream;

AudioResource::AudioResource(ResourceManager& manager, const String& uniqueID) : BaseClass(manager, uniqueID) {
	Flag().fetch_or(RESOURCE_STREAM, std::memory_order_acquire);
}

void AudioResource::Download(IFilterBase& device, void* deviceContext) {
}

void AudioResource::Upload(IFilterBase& device, void* deviceContext) {
	// no operations ...
}

void AudioResource::Attach(IFilterBase& device, void* deviceContext) {
	assert(audioStream == nullptr);
	audioStream = device.CreateFilter(rawStream)->QueryInterface(UniqueType<IAudio::Decoder>());
}

IAudio::Decoder* AudioResource::GetAudioStream() const {
	return audioStream;
}

void AudioResource::Detach(IFilterBase& device, void* deviceContext) {
	audioStream->ReleaseObject();
	audioStream = nullptr;
}

TObject<IReflect>& AudioResource::operator () (IReflect& reflect) {
	BaseClass::operator () (reflect);

	if (reflect.IsReflectProperty()) {
	}

	return *this;
}

bool AudioResource::operator << (IStreamBase& stream) {
	if (!(BaseClass::operator << (stream))) {
		return false;
	}

	// ignore payload, treat as online stream!
	IStreamBase& baseStream = stream.GetBaseStream();
	rawStream << baseStream;
	return true;
}

bool AudioResource::operator >> (IStreamBase& stream) const {
	if (!(BaseClass::operator >> (stream))) {
		return false;
	}

	IStreamBase& baseStream = stream.GetBaseStream();
	return rawStream >> baseStream;
}

IReflectObject* AudioResource::Clone() const {
	AudioResource* clone = new AudioResource(resourceManager, "");
	clone->rawStream = rawStream;
	return clone;
}
