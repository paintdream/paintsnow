#include "GraphicResourceBase.h"

using namespace PaintsNow;

GraphicResourceBase::GraphicResourceBase(ResourceManager& manager, const String& uniqueID) : BaseClass(manager, uniqueID) {}

GraphicResourceBase::~GraphicResourceBase() {}

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
