#include "WidgetRenderStage.h"

using namespace PaintsNow;
using namespace PaintsNow::NsMythForest;

WidgetRenderStage::WidgetRenderStage() : OutputColor(renderTargetDescription.colorBufferStorages[0]) {}

TObject<IReflect>& WidgetRenderStage::operator () (IReflect& reflect) {
	BaseClass::operator () (reflect);

	if (reflect.IsReflectProperty()) {
		ReflectProperty(Widgets);
		ReflectProperty(OutputColor);
	}

	return *this;
}

void WidgetRenderStage::PrepareResources(Engine& engine) {
	// prepare color buffers
	// OutputColor.renderTargetTextureResource = snowyStream.CreateReflectedResource(UniqueType<TextureResource>(), ResourceBase::GenerateRandomLocation("RT", &OutputColor), false, 0, nullptr);
	// OutputColor.renderTargetTextureResource->state.format = IRender::Resource::TextureDescription::UNSIGNED_BYTE;
	// OutputColor.renderTargetTextureResource->state.layout = IRender::Resource::TextureDescription::RGBA;
	renderTargetDescription.isBackBuffer = true;

	BaseClass::PrepareResources(engine);
}

void WidgetRenderStage::UpdatePass(Engine& engine) {
	BaseClass::UpdatePass(engine);
}
