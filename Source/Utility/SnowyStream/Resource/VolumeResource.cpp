#include "VolumeResource.h"

using namespace PaintsNow;
using namespace PaintsNow::NsSnowyStream;

VolumeResource::VolumeResource(ResourceManager& manager, const ResourceManager::UniqueLocation& uniqueID) : BaseClass(manager, uniqueID) {}

bool VolumeResource::operator << (IStreamBase& stream) {
	return false;
}

bool VolumeResource::operator >> (IStreamBase& stream) const {
	return false;
}

void VolumeResource::Attach(IRender& render, void* deviceContext) {
}

void VolumeResource::Detach(IRender& render, void* deviceContext) {

}

void VolumeResource::Upload(IRender& render, void* deviceContext) {

}

void VolumeResource::Download(IRender& render, void* deviceContext) {

}
