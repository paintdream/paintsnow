#include "LightBufferRenderStage.h"
#include "../../../Engine.h"
#include "../../../../SnowyStream/SnowyStream.h"
#include <sstream>

using namespace PaintsNow;

LightBufferRenderStage::LightBufferRenderStage(const String& config) : LightTexture(renderTargetDescription.colorStorages[0]) {
	uint8_t shift = Math::Min((uint8_t)atoi(config.c_str()), (uint8_t)16);
	resolutionShift = Char2((char)shift, (char)shift);
}

TObject<IReflect>& LightBufferRenderStage::operator () (IReflect& reflect) {
	BaseClass::operator () (reflect);

	if (reflect.IsReflectProperty()) {
		ReflectProperty(CameraView);
		ReflectProperty(InputDepth);
		ReflectProperty(LightSource);
		ReflectProperty(LightTexture);
	}

	return *this;
}

void LightBufferRenderStage::PrepareResources(Engine& engine, IRender::Queue* queue) {
	SnowyStream& snowyStream = engine.snowyStream;
	LightTexture.renderTargetDescription.state.format = IRender::Resource::TextureDescription::UNSIGNED_BYTE;
	LightTexture.renderTargetDescription.state.layout = IRender::Resource::TextureDescription::RGBA;
	LightTexture.renderTargetDescription.state.sample = IRender::Resource::TextureDescription::POINT;
	LightTexture.renderTargetDescription.state.immutable = false;
	LightTexture.renderTargetDescription.state.attachment = true;

	BaseClass::PrepareResources(engine, queue);
}

void LightBufferRenderStage::Uninitialize(Engine& engine, IRender::Queue* queue) {
	IRender& render = engine.interfaces.render;
	BaseClass::Uninitialize(engine, queue);
}

void LightBufferRenderStage::UpdatePass(Engine& engine, IRender::Queue* queue) {
	LightBufferPass& Pass = GetPass();
	ScreenTransformVS& screenTransform = Pass.transform;
	screenTransform.vertexBuffer.resource = meshResource->bufferCollection.positionBuffer;
	LightEncoderFS& encoder = Pass.encoder;
	encoder.depthTexture.resource = InputDepth.textureResource->GetRenderResource();
	encoder.inverseProjectionMatrix = CameraView->inverseProjectionMatrix;

	// Prepare lights
	const std::vector<RenderPortLightSource::LightElement>& lights = LightSource.lightElements;
	// get depth texture size
	const UShort3& dim = InputDepth.textureResource->description.dimension;
	// assume Full Screen Size = 1920 * 1080
	// and depth bounding with 8x8 grid
	// then depth texture is 240 * 135
	// then soft rasterizer size is 30 * 18 = 540

	// prepare soft rasterizer
	Float4* lightInfos = &encoder.lightInfos[0];
	encoder.invScreenSize = Float2(dim.x() == 0 ? 0 : 1.0f / dim.x(), dim.y() == 0 ? 0 : 1.0f / dim.y());
	int32_t gridWidth = (dim.x() + 7) >> 3, gridHeight = (dim.y() + 7) >> 3;
	uint32_t count = Math::Min((uint32_t)lights.size(), (uint32_t)LightEncoderFS::MAX_LIGHT_COUNT);
	encoder.lightCount = (float)count;

	MatrixFloat3x3 normalMatrix;

	for (uint32_t j = 0; j < 3; j++) {
		for (uint32_t i = 0; i < 3; i++) {
			normalMatrix(i, j) = CameraView->viewMatrix(i, j);
		}
	}

	for (uint32_t i = 0; i < count; i++) {
		const RenderPortLightSource::LightElement& light = lights[i];
		Float3 p(light.position.x(), light.position.y(), light.position.z());
		if (light.position.w() != 0) {
			p = Math::Transform(CameraView->viewMatrix, p);
		} else {
			p = Math::Normalize(p * normalMatrix);
		}

		lightInfos[i] = Float4(p.x(), p.y(), p.z(), light.position.w());
	}

	// assemble block data
	BaseClass::UpdatePass(engine, queue);
}
