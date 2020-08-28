#include "BloomRenderStage.h"

using namespace PaintsNow;

BloomRenderStage::BloomRenderStage(const String& config) : OutputColor(renderTargetDescription.colorStorages[0]) {
	uint8_t shift = Math::Min((uint8_t)atoi(config.c_str()), (uint8_t)16);
	resolutionShift = Char2((char)shift, (char)shift);
}

TObject<IReflect>& BloomRenderStage::operator () (IReflect& reflect) {
	BaseClass::operator () (reflect);

	if (reflect.IsReflectProperty()) {
		ReflectProperty(InputColor);
		ReflectProperty(OutputColor);
	}
	return *this;
}

void BloomRenderStage::PrepareResources(Engine& engine, IRender::Queue* queue) {
	SnowyStream& snowyStream = engine.snowyStream;
	OutputColor.renderTargetTextureResource = snowyStream.CreateReflectedResource(UniqueType<TextureResource>(), ResourceBase::GenerateLocation("RT", &OutputColor), false, 0, nullptr);
	OutputColor.renderTargetTextureResource->description.state.format = IRender::Resource::TextureDescription::HALF;
	OutputColor.renderTargetTextureResource->description.state.layout = IRender::Resource::TextureDescription::RGB;
	OutputColor.renderTargetTextureResource->description.state.immutable = false;
	OutputColor.renderTargetTextureResource->description.state.attachment = true;

	BaseClass::PrepareResources(engine, queue);
}

void BloomRenderStage::UpdatePass(Engine& engine, IRender::Queue* queue) {
	BloomPass& Pass = GetPass();
	ScreenTransformVS& screenTransform = Pass.screenTransform;
	screenTransform.vertexBuffer.resource = quadMeshResource->bufferCollection.positionBuffer;
	
	BloomFS& bloom = Pass.screenBloom;
	bloom.screenTexture.resource = InputColor.textureResource->GetTexture();
	const UShort3& dim = OutputColor.renderTargetTextureResource->description.dimension;
	bloom.invScreenSize = Float2(dim.x() == 0 ? 0 : 1.0f / dim.x(), dim.y() == 0 ? 0 : 1.0f / dim.y());

	BaseClass::UpdatePass(engine, queue);
}
