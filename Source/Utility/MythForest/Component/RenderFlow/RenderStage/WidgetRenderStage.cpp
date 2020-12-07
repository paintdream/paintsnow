#include "WidgetRenderStage.h"

using namespace PaintsNow;

WidgetRenderStage::WidgetRenderStage(const String& s) : OutputColor(renderTargetDescription.colorStorages[0]), InputColor(renderTargetDescription.colorStorages[0]) {
	renderTargetDescription.colorStorages[0].loadOp = IRender::Resource::RenderTargetDescription::DEFAULT;
	renderTargetDescription.colorStorages[0].storeOp = IRender::Resource::RenderTargetDescription::DEFAULT;
}

TObject<IReflect>& WidgetRenderStage::operator () (IReflect& reflect) {
	BaseClass::operator () (reflect);

	if (reflect.IsReflectProperty()) {
		ReflectProperty(Widgets);
		ReflectProperty(InputColor);
		ReflectProperty(OutputColor);
	}

	return *this;
}

void WidgetRenderStage::Prepare(Engine& engine, IRender::Queue* queue) {
	OutputColor.renderTargetDescription.state.format = IRender::Resource::TextureDescription::UNSIGNED_BYTE;
	OutputColor.renderTargetDescription.state.layout = IRender::Resource::TextureDescription::RGBA;

	BaseClass::Prepare(engine, queue);
}

void WidgetRenderStage::Update(Engine& engine, IRender::Queue* queue) {
	BaseClass::Update(engine, queue);
}
