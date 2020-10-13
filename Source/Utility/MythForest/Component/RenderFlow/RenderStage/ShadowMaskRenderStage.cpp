#include "ShadowMaskRenderStage.h"
#include "../../../Engine.h"
#include "../../../../SnowyStream/SnowyStream.h"
#include <sstream>

using namespace PaintsNow;

ShadowMaskRenderStage::ShadowMaskRenderStage(const String& config) : OutputMask(renderTargetDescription.colorStorages[0]) {}

TObject<IReflect>& ShadowMaskRenderStage::operator () (IReflect& reflect) {
	BaseClass::operator () (reflect);

	if (reflect.IsReflectProperty()) {
		ReflectProperty(LightSource);
		ReflectProperty(CameraView);
		ReflectProperty(InputDepth);
		ReflectProperty(OutputMask);
	}

	return *this;
}

void ShadowMaskRenderStage::PrepareResources(Engine& engine, IRender::Queue* queue) {
	SnowyStream& snowyStream = engine.snowyStream;
	OutputMask.renderTargetDescription.state.format = IRender::Resource::TextureDescription::UNSIGNED_BYTE;
	OutputMask.renderTargetDescription.state.layout = IRender::Resource::TextureDescription::R;
	OutputMask.renderTargetDescription.state.immutable = false;
	OutputMask.renderTargetDescription.state.attachment = true;

	emptyShadowMask = snowyStream.CreateReflectedResource(UniqueType<TextureResource>(), "[Runtime]/TextureResource/Black", true, 0, nullptr);

	BaseClass::PrepareResources(engine, queue);
}

void ShadowMaskRenderStage::UpdatePass(Engine& engine, IRender::Queue* queue) {
	ShadowMaskPass& Pass = GetPass();
	ScreenTransformVS& screenTransform = Pass.transform;
	screenTransform.vertexBuffer.resource = quadMeshResource->bufferCollection.positionBuffer;
	ShadowMaskFS& mask = Pass.mask;
	mask.depthTexture.resource = InputDepth.textureResource->GetRenderResource();
	mask.shadowTexture0.resource = emptyShadowMask->GetRenderResource();
	mask.shadowTexture1.resource = emptyShadowMask->GetRenderResource();
	mask.shadowTexture2.resource = emptyShadowMask->GetRenderResource();

	MatrixFloat4x4 inverseMatrix = CameraView->inverseProjectionMatrix * CameraView->inverseViewMatrix;

	for (size_t i = 0; i < LightSource->lightElements.size(); i++) {
		RenderPortLightSource::LightElement& element = LightSource->lightElements[i];
		// just get first shadow
		for (size_t j = 0; j < element.shadows.size(); j++) {
			RenderPortLightSource::LightElement::Shadow& shadow = element.shadows[j];
			if (shadow.shadowTexture) {
				switch (j) {
				case 0:
					mask.reprojectionMatrix0 = inverseMatrix * shadow.shadowMatrix;
					mask.shadowTexture0.resource = shadow.shadowTexture->GetRenderResource();
					break;
				case 1:
					mask.reprojectionMatrix1 = inverseMatrix * shadow.shadowMatrix;
					mask.shadowTexture1.resource = shadow.shadowTexture->GetRenderResource();
					break;
				case 2:
					mask.reprojectionMatrix2 = inverseMatrix * shadow.shadowMatrix;
					mask.shadowTexture2.resource = shadow.shadowTexture->GetRenderResource();
					break;
				}
			}
		}
	}

	// assert(LightSource->lightElements.empty() || mask.shadowTexture.resource != emptyShadowMask->GetTexture());
	BaseClass::UpdatePass(engine, queue);
}
