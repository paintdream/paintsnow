#include "DepthResolveRenderStage.h"
#include "../../../Engine.h"
#include "../../../../SnowyStream/SnowyStream.h"
#include <sstream>

using namespace PaintsNow;

DepthResolveRenderStage::DepthResolveRenderStage(const String& config) : OutputDepth(renderTargetDescription.colorBufferStorages[0]) {}

TObject<IReflect>& DepthResolveRenderStage::operator () (IReflect& reflect) {
	BaseClass::operator () (reflect);

	if (reflect.IsReflectProperty()) {
		ReflectProperty(InputDepth);
		ReflectProperty(OutputDepth);
	}

	return *this;
}

void DepthResolveRenderStage::PrepareResources(Engine& engine, IRender::Queue* queue) {
	SnowyStream& snowyStream = engine.snowyStream;
	OutputDepth.renderTargetTextureResource = snowyStream.CreateReflectedResource(UniqueType<TextureResource>(), ResourceBase::GenerateLocation("RT", &OutputDepth), false, 0, nullptr);
	OutputDepth.renderTargetTextureResource->description.state.format = IRender::Resource::TextureDescription::FLOAT;
	OutputDepth.renderTargetTextureResource->description.state.layout = IRender::Resource::TextureDescription::R;
	OutputDepth.renderTargetTextureResource->description.state.immutable = false;
	OutputDepth.renderTargetTextureResource->description.state.attachment = true;

	BaseClass::PrepareResources(engine, queue);
}

void DepthResolveRenderStage::UpdatePass(Engine& engine, IRender::Queue* queue) {
	DepthResolvePass& Pass = GetPass();
	ScreenTransformVS& screenTransform = Pass.transform;
	screenTransform.vertexBuffer.resource = quadMeshResource->bufferCollection.positionBuffer;
	DepthResolveFS& resolve = Pass.resolve;
	resolve.depthTexture.resource = InputDepth.textureResource->GetTexture();

	BaseClass::UpdatePass(engine, queue);
}
