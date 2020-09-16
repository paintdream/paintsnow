#include "RenderPortTextureInput.h"
#include "RenderPortRenderTarget.h"
#include "../RenderStage.h"

using namespace PaintsNow;

// RenderPortTextureInput

RenderPortTextureInput::RenderPortTextureInput() {}

TObject<IReflect>& RenderPortTextureInput::operator () (IReflect& reflect) {
	BaseClass::operator () (reflect);

	if (reflect.IsReflectProperty()) {
		ReflectProperty(textureResource);
	}

	return *this;
}

void RenderPortTextureInput::Initialize(IRender& render, IRender::Queue* mainQueue) {}
void RenderPortTextureInput::Uninitialize(IRender& render, IRender::Queue* mainQueue) {}
