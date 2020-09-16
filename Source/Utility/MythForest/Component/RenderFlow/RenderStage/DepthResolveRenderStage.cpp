#include "DepthResolveRenderStage.h"
#include "../../../Engine.h"
#include "../../../../SnowyStream/SnowyStream.h"
#include <sstream>

using namespace PaintsNow;

DepthResolveRenderStage::DepthResolveRenderStage(const String& config) : OutputDepth(renderTargetDescription.colorStorages[0]) {}

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
	OutputDepth.renderTargetDescription.state.format = IRender::Resource::TextureDescription::FLOAT;
	OutputDepth.renderTargetDescription.state.layout = IRender::Resource::TextureDescription::R;
	OutputDepth.renderTargetDescription.state.immutable = false;
	OutputDepth.renderTargetDescription.state.attachment = true;

	BaseClass::PrepareResources(engine, queue);
}

void DepthResolveRenderStage::UpdatePass(Engine& engine, IRender::Queue* queue) {
	DepthResolvePass& Pass = GetPass();
	ScreenTransformVS& screenTransform = Pass.transform;
	screenTransform.vertexBuffer.resource = quadMeshResource->bufferCollection.positionBuffer;
	DepthResolveFS& resolve = Pass.resolve;
	resolve.depthTexture.resource = InputDepth.textureResource->GetRenderResource();

	BaseClass::UpdatePass(engine, queue);
}
