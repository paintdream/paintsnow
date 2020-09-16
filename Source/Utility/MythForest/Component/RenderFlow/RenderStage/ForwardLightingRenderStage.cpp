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

void ForwardLightingRenderStage::PrepareResources(Engine& engine, IRender::Queue* queue) {
	IRender& render = engine.interfaces.render;
	SnowyStream& snowyStream = engine.snowyStream;
	OutputColor.renderTargetDescription = snowyStream.CreateReflectedResource(UniqueType<TextureResource>(), ResourceBase::GenerateLocation("RT", &OutputColor), false, 0, nullptr);
	OutputColor.renderTargetDescription.state.format = IRender::Resource::TextureDescription::HALF;
	OutputColor.renderTargetDescription.state.layout = IRender::Resource::TextureDescription::RGBA;
	OutputColor.renderTargetDescription.state.immutable = false;
	OutputColor.renderTargetDescription.state.attachment = true;

	BaseClass::PrepareResources(engine, queue);
}

void ForwardLightingRenderStage::UpdatePass(Engine& engine, IRender::Queue* queue) {
	BaseClass::UpdatePass(engine, queue);
}
