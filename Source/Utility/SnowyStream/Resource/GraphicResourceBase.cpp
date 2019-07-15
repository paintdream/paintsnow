#include "GraphicResourceBase.h"

using namespace PaintsNow;
using namespace PaintsNow::NsSnowyStream;

GraphicResourceBase::GraphicResourceBase(ResourceManager& manager, const ResourceManager::UniqueLocation& uniqueID) : BaseClass(manager, uniqueID) {}

GraphicResourceBase::~GraphicResourceBase() {}

void GraphicResourceBase::Attach(IRender& render, void* deviceContext) {
}


void GraphicResourceBase::Detach(IRender& render, void* deviceContext) {
}


TObject<IReflect>& GraphicResourceBase::operator () (IReflect& reflect) {
	BaseClass::operator () (reflect);

	return *this;
}
