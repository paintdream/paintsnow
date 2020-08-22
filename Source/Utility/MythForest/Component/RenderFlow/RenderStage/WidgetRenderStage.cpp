#include "WidgetRenderStage.h"

using namespace PaintsNow;

WidgetRenderStage::WidgetRenderStage(const String& s) : OutputColor(renderTargetDescription.colorBufferStorages[0]) {}

TObject<IReflect>& WidgetRenderStage::operator () (IReflect& reflect) {
	BaseClass::operator () (reflect);

	if (reflect.IsReflectProperty()) {
		ReflectProperty(Widgets);
		ReflectProperty(OutputColor);
	}

	return *this;
}

void WidgetRenderStage::PrepareResources(Engine& engine, IRender::Queue* queue) {
	// prepare color buffers
	// OutputColor.renderTargetTextureResource = snowyStream.CreateReflectedResource(UniqueType<TextureResource>(), ResourceBase::GenerateRandomLocation("RT", &OutputColor), false, 0, nullptr);
	// OutputColor.renderTargetTextureResource->state.format = IRender::Resource::TextureDescription::UNSIGNED_BYTE;
	// OutputColor.renderTargetTextureResource->state.layout = IRender::Resource::TextureDescription::RGBA;
	BaseClass::PrepareResources(engine, queue);
}

void WidgetRenderStage::UpdatePass(Engine& engine, IRender::Queue* queue) {
	BaseClass::UpdatePass(engine, queue);
}
