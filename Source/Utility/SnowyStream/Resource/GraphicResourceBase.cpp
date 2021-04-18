#include "GraphicResourceBase.h"

using namespace PaintsNow;

GraphicResourceBase::GraphicResourceBase(ResourceManager& manager, const String& uniqueID) : BaseClass(manager, uniqueID) {
	runtimeVersion.store(0, std::memory_order_relaxed);
}

GraphicResourceBase::~GraphicResourceBase() {}

bool GraphicResourceBase::Complete(size_t version) {
	size_t currentVersion = runtimeVersion.load(std::memory_order_acquire);
	if (currentVersion > version) return false;

	return runtimeVersion.compare_exchange_strong(currentVersion, version, std::memory_order_acquire);
}

void GraphicResourceBase::Refresh(IRender& render, void* deviceContext) {

}

void GraphicResourceBase::Attach(IRender& render, void* deviceContext) {
}

void GraphicResourceBase::Detach(IRender& render, void* deviceContext) {
}

TObject<IReflect>& GraphicResourceBase::operator () (IReflect& reflect) {
	BaseClass::operator () (reflect);

	return *this;
}
