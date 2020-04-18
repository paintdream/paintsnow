#include "ForwardLightingRenderStage.h"
#include "../../../../SnowyStream/SnowyStream.h"
#include <sstream>

using namespace PaintsNow;
using namespace PaintsNow::NsMythForest;
using namespace PaintsNow::NsSnowyStream;

ForwardLightingRenderStage::ForwardLightingRenderStage() : OutputColor(renderTargetDescription.colorBufferStorages[0]) {}

TObject<IReflect>& ForwardLightingRenderStage::operator () (IReflect& reflect) {
	BaseClass::operator () (reflect);

	if (reflect.IsReflectProperty()) {
		ReflectProperty(LightSource);
		ReflectProperty(Primitives);

		ReflectProperty(OutputColor);
	}

	return *this;
}

void ForwardLightingRenderStage::PrepareResources(Engine& engine) {
	IRender& render = engine.interfaces.render;
	SnowyStream& snowyStream = engine.snowyStream;
	OutputColor.renderTargetTextureResource = snowyStream.CreateReflectedResource(UniqueType<TextureResource>(), ResourceBase::GenerateRandomLocation("RT", &OutputColor), false, 0, nullptr);
	OutputColor.renderTargetTextureResource->description.state.format = IRender::Resource::TextureDescription::HALF_FLOAT;
	OutputColor.renderTargetTextureResource->description.state.layout = IRender::Resource::TextureDescription::RGBA;
	OutputColor.renderTargetTextureResource->description.state.immutable = false;

	BaseClass::PrepareResources(engine);
}

void ForwardLightingRenderStage::UpdatePass(Engine& engine) {
	BaseClass::UpdatePass(engine);
}
