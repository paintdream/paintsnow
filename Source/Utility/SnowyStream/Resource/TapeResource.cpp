#include "TapeResource.h"

using namespace PaintsNow;
using namespace PaintsNow::NsSnowyStream;

TapeResource::TapeResource(ResourceManager& manager, const ResourceManager::UniqueLocation& uniqueID) : BaseClass(manager, uniqueID), packetProvider(nullptr), packetStream(nullptr) {
	Flag() |= RESOURCE_STREAM;
}
TapeResource::~TapeResource() {
}

void TapeResource::Close() {
	if (packetProvider != nullptr) {
		delete packetProvider;
		packetProvider = nullptr;
	}

	if (packetStream != nullptr) {
		packetStream->ReleaseObject();
		packetStream = nullptr;
	}
}

uint64_t TapeResource::GetMemoryUsage() const {
	return 0;
}

void TapeResource::Download(IArchive& archive, void* deviceContext) {
	Close();

	size_t length;
	packetStream = archive.Open(streamPath, true, length, nullptr);
	if (packetStream != nullptr) {
		packetProvider = new ZPacket(*packetStream);
	}
}

void TapeResource::Upload(IArchive& archive, void* deviceContext) {
	// no operations ...
}

void TapeResource::Attach(IArchive& archive, void* deviceContext) {
}

void TapeResource::Detach(IArchive& archive, void* deviceContext) {
	Close();
}

ZPacket* TapeResource::GetPacket() {
	return packetProvider;
}