#include "DepthBoundingSetupRenderStage.h"
#include "../../../Engine.h"
#include "../../../../SnowyStream/SnowyStream.h"
#include <sstream>

using namespace PaintsNow;

DepthBoundingSetupRenderStage::DepthBoundingSetupRenderStage(const String& config) : OutputDepth(renderTargetDescription.colorStorages[0]) {
	uint8_t shift = Math::Min((uint8_t)atoi(config.c_str()), (uint8_t)16);
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

void DepthBoundingSetupRenderStage::PrepareResources(Engine& engine, IRender::Queue* queue) {
	SnowyStream& snowyStream = engine.snowyStream;
	OutputDepth.renderTargetTextureResource = snowyStream.CreateReflectedResource(UniqueType<TextureResource>(), ResourceBase::GenerateLocation("RT", &OutputDepth), false, 0, nullptr);
	OutputDepth.renderTargetTextureResource->description.state.format = IRender::Resource::TextureDescription::HALF;
	OutputDepth.renderTargetTextureResource->description.state.layout = IRender::Resource::TextureDescription::RG;
	OutputDepth.renderTargetTextureResource->description.state.sample = IRender::Resource::TextureDescription::POINT;
	OutputDepth.renderTargetTextureResource->description.state.immutable = false;
	OutputDepth.renderTargetTextureResource->description.state.attachment = true;

	BaseClass::PrepareResources(engine, queue);
}

void DepthBoundingSetupRenderStage::UpdatePass(Engine& engine, IRender::Queue* queue) {
	DepthBoundingSetupPass& Pass = GetPass();
	ScreenTransformVS& screenTransform = Pass.transform;
	screenTransform.vertexBuffer.resource = quadMeshResource->bufferCollection.positionBuffer;
	DepthMinMaxSetupFS& minmax = Pass.minmax;
	minmax.depthTexture.resource = InputDepth.textureResource->GetTexture();
	const UShort3& dim = OutputDepth.renderTargetTextureResource->description.dimension;
	minmax.invScreenSize = Float2(dim.x() == 0 ? 0 : 1.0f / dim.x(), dim.y() == 0 ? 0 : 1.0f / dim.y());
	BaseClass::UpdatePass(engine, queue);
}
