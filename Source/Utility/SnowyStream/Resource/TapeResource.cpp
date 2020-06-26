#include "TapeResource.h"

using namespace PaintsNow;
using namespace PaintsNow::NsSnowyStream;

TapeResource::TapeResource(ResourceManager& manager, const String& uniqueID) : BaseClass(manager, uniqueID), packet(nullptr) {
	Flag().fetch_or(RESOURCE_STREAM, std::memory_order_acquire);
}
TapeResource::~TapeResource() {
	Close();
}

void TapeResource::Close() {
	if (packet != nullptr) {
		delete packet;
		packet = nullptr;
	}
}

void TapeResource::Download(IArchive& archive, void* deviceContext) {
	// no operations ...
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
	return packet;
}


bool TapeResource::operator << (IStreamBase& stream) {
	if (!(BaseClass::operator << (stream))) {
		return false;
	}

	// ignore payload, treat as online stream!
	IStreamBase& baseStream = stream.GetBaseStream();
	localStream << baseStream;
	return true;
}

bool TapeResource::operator >> (IStreamBase& stream) const {
	if (!(BaseClass::operator >> (stream))) {
		return false;
	}

	IStreamBase& baseStream = stream.GetBaseStream();
	return localStream >> baseStream;
}


IReflectObject* TapeResource::Clone() const {
	TapeResource* clone = new TapeResource(resourceManager, "");
	// TODO:
	return clone;
}
