#include "RenderPortRenderTarget.h"

using namespace PaintsNow;
using namespace PaintsNow::NsMythForest;

// RenderPortRenderTarget
RenderPortRenderTarget::RenderPortRenderTarget(IRender::Resource::RenderTargetDescription::Storage& storage) : bindingStorage(storage) {}

TObject<IReflect>& RenderPortRenderTarget::operator () (IReflect& reflect) {
	BaseClass::operator () (reflect);

	if (reflect.IsReflectProperty()) {
		ReflectProperty(renderTargetTextureResource);
	}

	return *this;
}

void RenderPortRenderTarget::Initialize(IRender& render, IRender::Queue* mainQueue) {
}

void RenderPortRenderTarget::Uninitialize(IRender& render, IRender::Queue* mainQueue) {
}

bool RenderPortRenderTarget::UpdateDataStream(RenderPort& source) {
	// Sync texture
	RenderPortRenderTarget* target = source.QueryInterface(UniqueType<RenderPortRenderTarget>());
	if (target != nullptr) {
		renderTargetTextureResource = target->renderTargetTextureResource;
		return true;
	} else {
		return false;
	}
}