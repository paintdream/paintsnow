#include "DeferredLightingRenderStage.h"
#include "../../../../SnowyStream/SnowyStream.h"
#include <sstream>

using namespace PaintsNow;

DeferredLightingRenderStage::DeferredLightingRenderStage(const String& s) : OutputColor(renderTargetDescription.colorStorages[0]), LoadDepth(renderTargetDescription.depthStorage) {
	renderStateDescription.stencilTest = IRender::Resource::RenderStateDescription::NEVER;
	renderTargetDescription.colorStorages[0].loadOp = IRender::Resource::RenderTargetDescription::CLEAR;
	renderTargetDescription.colorStorages[0].storeOp = IRender::Resource::RenderTargetDescription::DEFAULT;
	renderTargetDescription.depthStorage.loadOp = IRender::Resource::RenderTargetDescription::DEFAULT;
	renderTargetDescription.depthStorage.storeOp = IRender::Resource::RenderTargetDescription::DISCARD;
	renderTargetDescription.stencilStorage.loadOp = IRender::Resource::RenderTargetDescription::DEFAULT;
	renderTargetDescription.stencilStorage.storeOp = IRender::Resource::RenderTargetDescription::DISCARD;
}

TObject<IReflect>& DeferredLightingRenderStage::operator () (IReflect& reflect) {
	BaseClass::operator () (reflect);

	if (reflect.IsReflectProperty()) {
		ReflectProperty(CameraView);
		ReflectProperty(LightSource);

		ReflectProperty(LightTexture);
		ReflectProperty(BaseColorOcclusion);
		ReflectProperty(NormalRoughnessMetallic);
		ReflectProperty(Depth);
		ReflectProperty(ShadowTexture);

		ReflectProperty(LoadDepth);
		ReflectProperty(OutputColor);
	}

	return *this;
}

void DeferredLightingRenderStage::PrepareResources(Engine& engine, IRender::Queue* queue) {
	IRender& render = engine.interfaces.render;
	SnowyStream& snowyStream = engine.snowyStream;
	OutputColor.renderTargetDescription.state.format = IRender::Resource::TextureDescription::HALF;
	OutputColor.renderTargetDescription.state.layout = IRender::Resource::TextureDescription::RGBA;
	OutputColor.renderTargetDescription.state.immutable = false;
	OutputColor.renderTargetDescription.state.attachment = true;

	BaseClass::PrepareResources(engine, queue);
}

void DeferredLightingRenderStage::UpdatePass(Engine& engine, IRender::Queue* queue) {
	DeferredLightingPass& Pass = GetPass();
	ScreenTransformVS& screenTransform = Pass.screenTransform;
	screenTransform.vertexBuffer.resource = meshResource->bufferCollection.positionBuffer;
	DeferredCompactDecodeFS& compactDecode = Pass.deferredCompactDecode;
	compactDecode.BaseColorOcclusionTexture.resource = BaseColorOcclusion.textureResource->GetRenderResource();
	compactDecode.NormalRoughnessMetallicTexture.resource = NormalRoughnessMetallic.textureResource->GetRenderResource();
	compactDecode.DepthTexture.resource = Depth.textureResource->GetRenderResource();
	compactDecode.inverseProjectionMatrix = CameraView->inverseProjectionMatrix;
	compactDecode.ShadowTexture.resource = ShadowTexture.textureResource->GetRenderResource();

	if (renderStateDescription.stencilMask != LightSource->stencilMask) {
		renderStateDescription.stencilMask = LightSource->stencilMask;
		renderStateDescription.stencilValue = LightSource->stencilMask;
		renderStateDescription.stencilTest = LightSource->stencilMask != 0 ? IRender::Resource::RenderStateDescription::EQUAL : IRender::Resource::RenderStateDescription::NEVER;
		renderStateDescription.stencilWrite = 0;
		IRender& render = engine.interfaces.render;
		render.UploadResource(queue, renderState, &renderStateDescription);
	}

	StandardLightingFS& standardLighting = Pass.standardLighting;
	standardLighting.cubeLevelInv = 1.0f;
	standardLighting.cubeStrength = 1.0f;

	if (LightSource->cubeMapTexture) {
		standardLighting.specTexture.resource = LightSource->cubeMapTexture->GetRenderResource();
		standardLighting.cubeLevelInv = 1.0f / Math::Log2((uint32_t)LightSource->cubeMapTexture->description.dimension.x());
		standardLighting.cubeStrength = LightSource->cubeStrength;
	} else {
		standardLighting.specTexture.resource = BaseColorOcclusion.textureResource->GetRenderResource();
	}

	/*
	if (LightSource->skyMapTexture) {
		standardLighting.ambientTexture.resource = LightSource->skyMapTexture ? LightSource->skyMapTexture->GetTexture() : LightSource->cubeMapTexture->GetTexture();
	} else {
		// Temporary code, fail back
		standardLighting.ambientTexture.resource = standardLighting.specTexture.resource;
	}*/

	// fill light buffers
	standardLighting.invWorldNormalMatrix = CameraView->inverseViewMatrix;
	const std::vector<RenderPortLightSource::LightElement>& lights = LightSource->lightElements;
	uint32_t count = Math::Min((uint32_t)lights.size(), (uint32_t)StandardLightingFS::MAX_LIGHT_COUNT);
	for (uint32_t i = 0; i < count; i++) {
		const RenderPortLightSource::LightElement& light = lights[i];
		standardLighting.lightInfos[i * 2] = light.position;
		standardLighting.lightInfos[i * 2 + 1] = light.colorAttenuation;
	}

	standardLighting.lightCount = count;
	standardLighting.lightTexture.resource = LightTexture.textureResource->GetRenderResource();

	BaseClass::UpdatePass(engine, queue);
}
