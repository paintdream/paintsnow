#include "RenderResourceBase.h"

using namespace PaintsNow;

RenderResourceBase::RenderResourceBase(ResourceManager& manager, const String& uniqueID) : BaseClass(manager, uniqueID) {
	runtimeVersion.store(0, std::memory_order_relaxed);
}

RenderResourceBase::~RenderResourceBase() {}

bool RenderResourceBase::Complete(size_t version) {
	size_t currentVersion = runtimeVersion.load(std::memory_order_acquire);
	if (currentVersion > version) return false;

	return runtimeVersion.compare_exchange_strong(currentVersion, version, std::memory_order_acquire);
}

void RenderResourceBase::Refresh(IRender& render, void* deviceContext) {

}

void RenderResourceBase::Attach(IRender& render, void* deviceContext) {
}

void RenderResourceBase::Detach(IRender& render, void* deviceContext) {
}

TObject<IReflect>& RenderResourceBase::operator () (IReflect& reflect) {
	BaseClass::operator () (reflect);

	return *this;
}
