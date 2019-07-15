#include "DepthBoundingSetupRenderStage.h"
#include "../../../Engine.h"
#include "../../../../SnowyStream/SnowyStream.h"
#include <sstream>

using namespace PaintsNow;
using namespace PaintsNow::NsMythForest;
using namespace PaintsNow::NsSnowyStream;

DepthBoundingSetupRenderStage::DepthBoundingSetupRenderStage(const String& config) : OutputDepth(renderTargetDescription.colorBufferStorages[0]) {
	uint8_t shift = Min((uint8_t)atoi(config.c_str()), (uint8_t)16);
	resolutionShift = Char2((char)shift, (char)shift);
}

TObject<IReflect>& DepthBoundingSetupRenderStage::operator () (IReflect& reflect) {
	BaseClass::operator () (reflect);

	if (reflect.IsReflectProperty()) {
		ReflectProperty(InputDepth);
		ReflectProperty(OutputDepth);
	}

	return *this;
}

void DepthBoundingSetupRenderStage::PrepareResources(Engine& engine) {
	SnowyStream& snowyStream = engine.snowyStream;
	OutputDepth.renderTargetTextureResource = snowyStream.CreateReflectedResource(UniqueType<TextureResource>(), ResourceBase::GenerateRandomLocation("RT", &OutputDepth), false, 0, nullptr);
	OutputDepth.renderTargetTextureResource->description.state.format = IRender::Resource::TextureDescription::HALF_FLOAT;
	OutputDepth.renderTargetTextureResource->description.state.layout = IRender::Resource::TextureDescription::RG;
	OutputDepth.renderTargetTextureResource->description.state.sample = IRender::Resource::TextureDescription::POINT;

	BaseClass::PrepareResources(engine);
}

void DepthBoundingSetupRenderStage::UpdatePass(Engine& engine) {
	DepthBoundingSetupPass& Pass = GetPass();
	ScreenTransformVS& screenTransform = Pass.transform;
	screenTransform.vertexBuffer.resource = quadMeshResource->bufferCollection.positionBuffer;
	DepthMinMaxSetupFS& minmax = Pass.minmax;
	minmax.depthTexture.resource = InputDepth.textureResource->GetTexture();
	const UShort3& dim = OutputDepth.renderTargetTextureResource->description.dimension;
	minmax.invScreenSize = Float2(dim.x() == 0 ? 0 : 1.0f / dim.x(), dim.y() == 0 ? 0 : 1.0f / dim.y());
	BaseClass::UpdatePass(engine);
}
