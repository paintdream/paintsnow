#include "StencilMaskRenderStage.h"
#include "../../../Engine.h"
#include "../../../../SnowyStream/SnowyStream.h"
#include <sstream>

using namespace PaintsNow;
using namespace PaintsNow::NsMythForest;
using namespace PaintsNow::NsSnowyStream;

StencilMaskRenderStage::StencilMaskRenderStage(const String& config) : InputDepth(renderTargetDescription.depthStencilStorage), InputColorPlaceHolder(renderTargetDescription.colorBufferStorages[0]), OutputDepth(renderTargetDescription.depthStencilStorage) {
	renderStateDescription.colorWrite = 0;
	renderStateDescription.depthTest = 1;
	renderStateDescription.stencilTest = IRender::Resource::RenderStateDescription::ALWAYS;
	renderStateDescription.stencilWrite = 1;
	renderStateDescription.stencilValue = atoi(config.c_str());
	renderStateDescription.stencilMask = 0xff;

	clearDescription.clearColorBit = IRender::Resource::ClearDescription::DISCARD_LOAD | IRender::Resource::ClearDescription::DISCARD_STORE;
	clearDescription.clearDepthBit = clearDescription.clearStencilBit = 0; // load & store
}

TObject<IReflect>& StencilMaskRenderStage::operator () (IReflect& reflect) {
	BaseClass::operator () (reflect);

	if (reflect.IsReflectProperty()) {
		ReflectProperty(InputDepth);
		ReflectProperty(InputColorPlaceHolder);
		ReflectProperty(OutputDepth);
	}

	return *this;
}

void StencilMaskRenderStage::PrepareResources(Engine& engine) {
	SnowyStream& snowyStream = engine.snowyStream;
	// Do nothing

	BaseClass::PrepareResources(engine);
}

void StencilMaskRenderStage::UpdatePass(Engine& engine) {
	ConstMapPass& Pass = GetPass();
	// TODO: Bind Box Mesh ...

	BaseClass::UpdatePass(engine);
}
