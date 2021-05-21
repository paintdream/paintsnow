#include "ScreenSpaceTraceRenderStage.h"

using namespace PaintsNow;

ScreenSpaceTraceRenderStage::ScreenSpaceTraceRenderStage(const String& s) : ScreenCoord(renderTargetDescription.colorStorages[0]) {
	renderTargetDescription.colorStorages[0].loadOp = IRender::Resource::RenderTargetDescription::CLEAR;
	renderTargetDescription.colorStorages[0].storeOp = IRender::Resource::RenderTargetDescription::DEFAULT;

	renderStateDescription.depthTest = IRender::Resource::RenderStateDescription::GREATER;
	renderStateDescription.depthWrite = 1;
	renderStateDescription.stencilWrite = 1;
}


TObject<IReflect>& ScreenSpaceTraceRenderStage::operator () (IReflect& reflect) {
	BaseClass::operator () (reflect);

	if (reflect.IsReflectProperty()) {
		ReflectProperty(Depth);
		ReflectProperty(Normal);
		ReflectProperty(CameraView);
		ReflectProperty(ScreenCoord);
	}

	return *this;
}

void ScreenSpaceTraceRenderStage::Prepare(Engine& engine, IRender::Queue* queue) {
	IRender& render = engine.interfaces.render;
	SnowyStream& snowyStream = engine.snowyStream;

	ScreenCoord.renderTargetDescription.state.format = IRender::Resource::TextureDescription::UNSIGNED_SHORT;
	ScreenCoord.renderTargetDescription.state.layout = IRender::Resource::TextureDescription::RG;
	ScreenCoord.renderTargetDescription.state.sample = IRender::Resource::TextureDescription::POINT;
	ScreenCoord.renderTargetDescription.state.immutable = false;
	ScreenCoord.renderTargetDescription.state.attachment = true;

	BaseClass::Prepare(engine, queue);
}

void ScreenSpaceTraceRenderStage::Update(Engine& engine, IRender::Queue* queue) {
	ScreenSpaceTracePass& pass = GetPass();
	pass.shaderScreen.depthTexture.resource = Depth.textureResource->GetRenderResource();
	pass.shaderScreen.normalTexture.resource = Normal.textureResource->GetRenderResource();
	pass.shaderScreen.projectionMatrix = CameraView->projectionMatrix;
	pass.shaderScreen.inverseProjectionMatrix = CameraView->inverseProjectionMatrix;
	pass.screenTransform.vertexBuffer.resource = meshResource->bufferCollection.positionBuffer;

	const UShort3& dim = Depth.textureResource->description.dimension;
	pass.shaderScreen.invScreenSize = Float2(1.0f / dim.x(), 1.0f / dim.y());

	BaseClass::Update(engine, queue);
}
