#include "ShadowMaskRenderStage.h"
#include "../../../Engine.h"
#include "../../../../SnowyStream/SnowyStream.h"
#include <sstream>

using namespace PaintsNow;

ShadowMaskRenderStage::ShadowMaskRenderStage(const String& config) : OutputMask(renderTargetDescription.colorStorages[0]), InputMask(renderTargetDescription.colorStorages[0]) {
	layerIndex = atoi(config.c_str());
	IRender::Resource::RenderStateDescription& s = renderStateDescription;
	s.cullFrontFace = 1;
	s.depthTest = IRender::Resource::RenderStateDescription::DISABLED;
}

TObject<IReflect>& ShadowMaskRenderStage::operator () (IReflect& reflect) {
	BaseClass::operator () (reflect);

	if (reflect.IsReflectProperty()) {
		ReflectProperty(LightSource);
		ReflectProperty(CameraView);
		ReflectProperty(InputDepth);
		ReflectProperty(InputMask);
		ReflectProperty(OutputMask);
	}

	return *this;
}

void ShadowMaskRenderStage::PrepareResources(Engine& engine, IRender::Queue* queue) {
	SnowyStream& snowyStream = engine.snowyStream;

	if (InputMask.GetLinks().empty()) {
		OutputMask.renderTargetDescription.state.format = IRender::Resource::TextureDescription::UNSIGNED_BYTE;
		OutputMask.renderTargetDescription.state.layout = IRender::Resource::TextureDescription::R;
		OutputMask.renderTargetDescription.state.immutable = false;
		OutputMask.renderTargetDescription.state.attachment = true;

		renderTargetDescription.colorStorages[0].loadOp = IRender::Resource::RenderTargetDescription::CLEAR;
	}

	emptyShadowMask = snowyStream.CreateReflectedResource(UniqueType<TextureResource>(), "[Runtime]/TextureResource/Black", true, 0, nullptr);

	const String path = "[Runtime]/MeshResource/StandardCube";
	meshResource = engine.snowyStream.CreateReflectedResource(UniqueType<MeshResource>(), path, true, 0, nullptr);
	assert(meshResource->Flag() & ResourceBase::RESOURCE_UPLOADED);

	BaseClass::PrepareResources(engine, queue);
}

void ShadowMaskRenderStage::UpdatePass(Engine& engine, IRender::Queue* queue) {
	ShadowMaskPass& Pass = GetPass();
	ScreenTransformVS& screenTransform = Pass.transform;
	screenTransform.vertexBuffer.resource = meshResource->bufferCollection.positionBuffer;
	ShadowMaskFS& mask = Pass.mask;
	mask.depthTexture.resource = InputDepth.textureResource->GetRenderResource();
	mask.shadowTexture.resource = emptyShadowMask->GetRenderResource();

	MatrixFloat4x4 inverseMatrix = CameraView->inverseProjectionMatrix * CameraView->inverseViewMatrix;

	for (size_t i = 0; i < LightSource->lightElements.size(); i++) {
		RenderPortLightSource::LightElement& element = LightSource->lightElements[i];
		// just get first shadow
		if (layerIndex < element.shadows.size()) {
			RenderPortLightSource::LightElement::Shadow& shadow = element.shadows[layerIndex];
			mask.reprojectionMatrix = inverseMatrix * shadow.shadowMatrix;

			if (shadow.shadowTexture) {
				mask.shadowTexture.resource = shadow.shadowTexture->GetRenderResource();
			}
		}
	}

	// assert(LightSource->lightElements.empty() || mask.shadowTexture.resource != emptyShadowMask->GetTexture());
	BaseClass::UpdatePass(engine, queue);
}
