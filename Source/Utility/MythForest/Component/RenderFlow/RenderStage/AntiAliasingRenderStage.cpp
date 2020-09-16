#include "AntiAliasingRenderStage.h"

using namespace PaintsNow;

AntiAliasingRenderStage::AntiAliasingRenderStage(const String& options) : OutputColor(renderTargetDescription.colorStorages[0]) {}

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
	OutputColor.renderTargetDescription.state.format = IRender::Resource::TextureDescription::HALF;
	OutputColor.renderTargetDescription.state.layout = IRender::Resource::TextureDescription::RGBA;
	OutputColor.renderTargetDescription.state.immutable = false;
	OutputColor.renderTargetDescription.state.attachment = true;

	BaseClass::PrepareResources(engine, queue);
}

void AntiAliasingRenderStage::UpdatePass(Engine& engine, IRender::Queue* queue) {
	// Do not update pass in UpdatePass(engine)
	AntiAliasingPass& Pass = GetPass();
	ScreenTransformVS& screenTransform = Pass.transform;
	screenTransform.vertexBuffer.resource = quadMeshResource->bufferCollection.positionBuffer;
	
	AntiAliasingFS& antiAliasing = Pass.antiAliasing;
	antiAliasing.inputTexture.resource = InputColor.textureResource->GetRenderResource();
	antiAliasing.lastInputTexture.resource = LastInputColor.textureResource->GetRenderResource();
	assert(antiAliasing.inputTexture.resource != antiAliasing.lastInputTexture.resource);
	// assert(OutputColor.renderTargetTextureResource->GetTexture() != antiAliasing.lastInputTexture.resource);
	antiAliasing.depthTexture.resource = Depth.textureResource->GetRenderResource();
	antiAliasing.reprojectionMatrix = CameraView->reprojectionMatrix;
	antiAliasing.unjitter = -CameraView->jitterOffset;

	const UShort3& dim = InputColor.textureResource->description.dimension;
	antiAliasing.invScreenSize = Float2(1.0f / dim.x(), 1.0f / dim.y());
	// std::swap(antiAliasing.unjitter, antiAliasing.invScreenSize);

	BaseClass::UpdatePass(engine, queue);
}

