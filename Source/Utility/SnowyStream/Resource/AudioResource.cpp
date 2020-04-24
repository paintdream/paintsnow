#include "AudioResource.h"

using namespace PaintsNow;
using namespace PaintsNow::NsSnowyStream;

AudioResource::AudioResource(ResourceManager& manager, const ResourceManager::UniqueLocation& uniqueID) : BaseClass(manager, uniqueID) {
	Flag() |= RESOURCE_STREAM;
}

void AudioResource::Download(IAudio& audio, void* deviceContext) {
}

void AudioResource::Upload(IAudio& audio, void* deviceContext) {
	// no operations ...
}

void AudioResource::Attach(IAudio& audio, void* deviceContext) {
}

void AudioResource::Detach(IAudio& audio, void* deviceContext) {
}

IAudio::Buffer* AudioResource::GetAudioBuffer() {
	return audioBuffer;
}