#include "ForwardLightingRenderStage.h"
#include "../../../../SnowyStream/SnowyStream.h"
#include <sstream>

using namespace PaintsNow;

ForwardLightingRenderStage::ForwardLightingRenderStage(const String& s) : OutputColor(renderTargetDescription.colorStorages[0]) {}

TObject<IReflect>& ForwardLightingRenderStage::operator () (IReflect& reflect) {
	BaseClass::operator () (reflect);

	if (reflect.IsReflectProperty()) {
		ReflectProperty(LightSource);
		ReflectProperty(Primitives);

		ReflectProperty(OutputColor);
	}

	return *this;
}

void ForwardLightingRenderStage::Prepare(Engine& engine, IRender::Queue* queue) {
	IRender& render = engine.interfaces.render;
	SnowyStream& snowyStream = engine.snowyStream;
	OutputColor.renderTargetDescription.state.format = IRender::Resource::TextureDescription::HALF;
	OutputColor.renderTargetDescription.state.layout = IRender::Resource::TextureDescription::RGBA;
	OutputColor.renderTargetDescription.state.immutable = false;
	OutputColor.renderTargetDescription.state.attachment = true;

	BaseClass::Prepare(engine, queue);
}

void ForwardLightingRenderStage::Update(Engine& engine, IRender::Queue* queue) {
	BaseClass::Update(engine, queue);
}
