#include "AntiAliasingRenderStage.h"

using namespace PaintsNow;
using namespace PaintsNow::NsMythForest;
using namespace PaintsNow::NsSnowyStream;

AntiAliasingRenderStage::AntiAliasingRenderStage(const String& options) : OutputColor(renderTargetDescription.colorBufferStorages[0]) {}

TObject<IReflect>& AntiAliasingRenderStage::operator () (IReflect& reflect) {
	BaseClass::operator () (reflect);

	if (reflect.IsReflectProperty()) {
		ReflectProperty(CameraView);

		ReflectProperty(InputColor);
		ReflectProperty(LastInputColor);
		ReflectProperty(Depth);
		ReflectProperty(OutputColor);
	}

	return *this;
}

void AntiAliasingRenderStage::PrepareResources(Engine& engine, IRender::Queue* queue) {
	SnowyStream& snowyStream = engine.snowyStream;
	OutputColor.renderTargetTextureResource = snowyStream.CreateReflectedResource(UniqueType<TextureResource>(), ResourceBase::GenerateLocation("RT", &OutputColor), false, 0, nullptr);
	OutputColor.renderTargetTextureResource->description.state.format = IRender::Resource::TextureDescription::HALF;
	OutputColor.renderTargetTextureResource->description.state.layout = IRender::Resource::TextureDescription::RGBA;
	OutputColor.renderTargetTextureResource->description.state.immutable = false;
	OutputColor.renderTargetTextureResource->description.state.attachment = true;

	BaseClass::PrepareResources(engine, queue);
}

void AntiAliasingRenderStage::UpdatePass(Engine& engine, IRender::Queue* queue) {
	// Do not update pass in UpdatePass(engine)
	AntiAliasingPass& Pass = GetPass();
	ScreenTransformVS& screenTransform = Pass.transform;
	screenTransform.vertexBuffer.resource = quadMeshResource->bufferCollection.positionBuffer;
	
	AntiAliasingFS& antiAliasing = Pass.antiAliasing;
	antiAliasing.inputTexture.resource = InputColor.textureResource->GetTexture();
	antiAliasing.lastInputTexture.resource = LastInputColor.textureResource->GetTexture();
	assert(antiAliasing.inputTexture.resource != antiAliasing.lastInputTexture.resource);
	// assert(OutputColor.renderTargetTextureResource->GetTexture() != antiAliasing.lastInputTexture.resource);
	antiAliasing.depthTexture.resource = Depth.textureResource->GetTexture();
	antiAliasing.reprojectionMatrix = CameraView->reprojectionMatrix;
	antiAliasing.unjitter = -CameraView->jitterOffset;

	const UShort3& dim = InputColor.textureResource->description.dimension;
	antiAliasing.invScreenSize = Float2(1.0f / dim.x(), 1.0f / dim.y());
	// std::swap(antiAliasing.unjitter, antiAliasing.invScreenSize);

	BaseClass::UpdatePass(engine, queue);
}

