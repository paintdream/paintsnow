#include "StreamResource.h"

using namespace PaintsNow;
using namespace PaintsNow::NsSnowyStream;

StreamResource::StreamResource(ResourceManager& manager, const String& uniqueID) : BaseClass(manager, uniqueID) {
	Flag().fetch_or(RESOURCE_STREAM, std::memory_order_acquire);
}

void StreamResource::Download(IArchive& archive, void* deviceContext) {}
void StreamResource::Upload(IArchive& archive, void* deviceContext) {}
void StreamResource::Attach(IArchive& archive, void* deviceContext) {}
void StreamResource::Detach(IArchive& archive, void* deviceContext) {}

IStreamBase& StreamResource::GetStream() {
	return stream;
}

bool StreamResource::operator << (IStreamBase& base) {
	if (!(BaseClass::operator << (base))) {
		return false;
	}

	stream << base.GetBaseStream();
	return true;
}

bool StreamResource::operator >> (IStreamBase& base) const {
	if (!(BaseClass::operator >> (base))) {
		return false;
	}

	return stream >> base.GetBaseStream();
}


IReflectObject* StreamResource::Clone() const {
	StreamResource* clone = new StreamResource(resourceManager, "");
	clone->stream = stream;
	return clone;
}