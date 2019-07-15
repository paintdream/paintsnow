#include "ParticleResource.h"

using namespace PaintsNow;
using namespace PaintsNow::NsSnowyStream;

ParticleResource::ParticleResource(ResourceManager& manager, const ResourceManager::UniqueLocation& uniqueID) : BaseClass(manager, uniqueID) {}

uint64_t ParticleResource::GetMemoryUsage() const {
	return 0;
}

bool ParticleResource::operator << (IStreamBase& stream) {
	return false;
}

bool ParticleResource::operator >> (IStreamBase& stream) const {
	return false;
}

void ParticleResource::Attach(IRender& render, void* deviceContext) {
}

void ParticleResource::Detach(IRender& render, void* deviceContext) {

}

void ParticleResource::Upload(IRender& render, void* deviceContext) {

}

void ParticleResource::Download(IRender& render, void* deviceContext) {

}
