#include "ScreenRenderStage.h"
#include "../../../Engine.h"
#include "../../../../SnowyStream/SnowyStream.h"
#include <sstream>

using namespace PaintsNow;
using namespace PaintsNow::NsMythForest;
using namespace PaintsNow::NsSnowyStream;

ScreenRenderStage::ScreenRenderStage(const String& config) : OutputColor(renderTargetDescription.colorBufferStorages[0]) {
	size_t count = Max(1, atoi(config.c_str()));
	BloomLayers.resize(count);
	for (size_t i = 0; i < count; i++) {
		BloomLayers[i] = TShared<RenderPortTextureInput>::From(new RenderPortTextureInput());
	}
}

TObject<IReflect>& ScreenRenderStage::operator () (IReflect& reflect) {
	BaseClass::operator () (reflect);
	
	if (reflect.IsReflectProperty()) {
		ReflectProperty(InputColor);
		ReflectProperty(BloomLayers);
		ReflectProperty(OutputColor);
	}

	return *this;
}

void ScreenRenderStage::PrepareResources(Engine& engine, IRender::Queue* queue) {
	IRender& render = engine.interfaces.render;
	SnowyStream& snowyStream = engine.snowyStream;
	OutputColor.renderTargetTextureResource = snowyStream.CreateReflectedResource(UniqueType<TextureResource>(), ResourceBase::GenerateRandomLocation("RT", &OutputColor), false, 0, nullptr);
	OutputColor.renderTargetTextureResource->description.state.format = IRender::Resource::TextureDescription::UNSIGNED_BYTE;
	OutputColor.renderTargetTextureResource->description.state.layout = IRender::Resource::TextureDescription::RGBA;
	OutputColor.renderTargetTextureResource->description.state.immutable = false;

	BaseClass::PrepareResources(engine, queue);
}

void ScreenRenderStage::UpdatePass(Engine& engine, IRender::Queue* queue) {
	ScreenPass& Pass = GetPass();
	ScreenTransformVS& screenTransform = Pass.screenTransform;
	screenTransform.vertexBuffer.resource = quadMeshResource->bufferCollection.positionBuffer;
	ScreenFS& ScreenFS = Pass.shaderScreen;
	ScreenFS.inputColorTexture.resource = InputColor.textureResource->GetTexture();
	assert(BloomLayers.size() > 0);
	
	ScreenFS.inputBloomTexture0.resource = BloomLayers[0]->textureResource->GetTexture();
	ScreenFS.inputBloomTexture1.resource = BloomLayers.size() > 1 ? BloomLayers[1]->textureResource->GetTexture() : ScreenFS.inputBloomTexture0.resource;
	ScreenFS.inputBloomTexture2.resource = BloomLayers.size() > 2 ? BloomLayers[2]->textureResource->GetTexture() : ScreenFS.inputBloomTexture1.resource;

	BaseClass::UpdatePass(engine, queue);
}
