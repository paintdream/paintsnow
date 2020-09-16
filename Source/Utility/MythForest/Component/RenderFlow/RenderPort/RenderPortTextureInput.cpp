#include "RenderPortTextureInput.h"
#include "RenderPortRenderTarget.h"
#include "../RenderStage.h"

using namespace PaintsNow;

// RenderPortTextureInput

RenderPortTextureInput::RenderPortTextureInput() : linkedRenderStage(nullptr) {}

TObject<IReflect>& RenderPortTextureInput::operator () (IReflect& reflect) {
	BaseClass::operator () (reflect);

	if (reflect.IsReflectProperty()) {
		ReflectProperty(textureResource);
	}

	return *this;
}

void RenderPortTextureInput::Initialize(IRender& render, IRender::Queue* mainQueue) {
}

void RenderPortTextureInput::Uninitialize(IRender& render, IRender::Queue* mainQueue) {
}

bool RenderPortTextureInput::UpdateDataStream(RenderPort& source) {
	// Sync texture, now supports fetch textue from RenderTarget only.
	RenderPortRenderTargetStore* target = source.QueryInterface(UniqueType<RenderPortRenderTargetStore>());
	if (target != nullptr) {
		textureResource = target->renderTargetTextureResource;
		linkedRenderStage = static_cast<RenderStage*>(source.GetNode());
		return true;
	} else {
		return false;
	}
}