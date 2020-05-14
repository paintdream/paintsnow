#include "FrameBarrierRenderStage.h"

using namespace PaintsNow;
using namespace PaintsNow::NsMythForest;
using namespace PaintsNow::NsSnowyStream;

FrameBarrierRenderStage::FrameBarrierRenderStage() : Next(renderTargetDescription.colorBufferStorages[0]) {
	Flag() &= ~RENDERSTAGE_ADAPT_MAIN_RESOLUTION;
	Front.Flag() |= RenderStage::RENDERSTAGE_WEAK_LINKAGE;
}

TObject<IReflect>& FrameBarrierRenderStage::operator () (IReflect& reflect) {
	BaseClass::operator () (reflect);

	if (reflect.IsReflectProperty()) {
		ReflectProperty(Front);
		ReflectProperty(Next);
	}

	return *this;
}

void FrameBarrierRenderStage::PrepareResources(Engine& engine, IRender::Queue* queue) {
	IRender& render = engine.interfaces.render;
	SnowyStream& snowyStream = engine.snowyStream;

	// Inititalize 
	Next.renderTargetTextureResource = snowyStream.CreateReflectedResource(UniqueType<TextureResource>(), ResourceBase::GenerateRandomLocation("RT", &Next), false, 0, nullptr);
	Next.renderTargetTextureResource->description.state.immutable = false;
	/*
	Next.renderTargetTextureResource->description.state.format = IRender::Resource::TextureDescription::UNSIGNED_BYTE;
	Next.renderTargetTextureResource->description.state.layout = IRender::Resource::TextureDescription::RGBA;
	*/
	// Copy descriptions from tracked render stage
	// RenderStage::PrepareResources(engine);
}

void FrameBarrierRenderStage::SetMainResolution(Engine& engine, IRender::Queue* queue, uint32_t width, uint32_t height, bool resizeOnly) {}
void FrameBarrierRenderStage::UpdateRenderTarget(Engine& engine, IRender::Queue* queue, bool resizeOnly) {}
void FrameBarrierRenderStage::UpdatePass(Engine& engine, IRender::Queue* queue) {}
void FrameBarrierRenderStage::Tick(Engine& engine, IRender::Queue* queue) {
	// Force update source 
	RenderStage* renderStage = Front.linkedRenderStage;
	if (renderStage != nullptr) {
		if (Front.textureResource->description.state != Next.renderTargetTextureResource->description.state
			|| renderStage->renderTargetDescription.width != Next.renderTargetTextureResource->description.dimension.x()
			|| renderStage->renderTargetDescription.height != Next.renderTargetTextureResource->description.dimension.y()) {

			Next.renderTargetTextureResource->description.state = Front.textureResource->description.state;
			Next.renderTargetTextureResource->description.dimension.x() = renderStage->renderTargetDescription.width;
			Next.renderTargetTextureResource->description.dimension.y() = renderStage->renderTargetDescription.height;
			Next.renderTargetTextureResource->GetResourceManager().InvokeUpload(Next.renderTargetTextureResource(), queue);
			Front.textureResource->GetResourceManager().InvokeUpload(Front.textureResource(), queue);
		}

		IRender::Resource* next = Next.renderTargetTextureResource->GetTexture();
		IRender::Resource* front = Front.textureResource->GetTexture();

		IRender& render = engine.interfaces.render;
		render.SwapResource(queue, next, front);
		IRender::Resource::RenderTargetDescription copy = renderStage->renderTargetDescription;
		render.UploadResource(queue, renderStage->renderTarget, &copy);
	}
}

