#include "FrameBarrierRenderStage.h"

using namespace PaintsNow;
using namespace PaintsNow::NsMythForest;
using namespace PaintsNow::NsSnowyStream;

FrameBarrierRenderStage::FrameBarrierRenderStage(const String& s) : Next(renderTargetDescription.colorBufferStorages[0]) {
	Flag().fetch_and(~RENDERSTAGE_ADAPT_MAIN_RESOLUTION, std::memory_order_release);
	Front.Flag().fetch_or(RenderStage::RENDERSTAGE_WEAK_LINKAGE, std::memory_order_acquire);
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
	Next.renderTargetTextureResource = snowyStream.CreateReflectedResource(UniqueType<TextureResource>(), ResourceBase::GenerateLocation("RT", &Next), false, 0, nullptr);
	Next.renderTargetTextureResource->description.state.immutable = false;
	Next.renderTargetTextureResource->description.state.attachment = true;
	/*
	Next.renderTargetTextureResource->description.state.format = IRender::Resource::TextureDescription::UNSIGNED_BYTE;
	Next.renderTargetTextureResource->description.state.layout = IRender::Resource::TextureDescription::RGBA;
	*/
	// Copy descriptions from tracked render stage
	// RenderStage::PrepareResources(engine);
}

void FrameBarrierRenderStage::SetMainResolution(Engine& engine, IRender::Queue* queue, uint32_t width, uint32_t height) {}
void FrameBarrierRenderStage::UpdatePass(Engine& engine, IRender::Queue* queue) {}
void FrameBarrierRenderStage::Commit(Engine& engine, std::vector<FencedRenderQueue*>& queues, std::vector<IRender::Queue*>& instantQueues, IRender::Queue* queue) {}

void FrameBarrierRenderStage::Tick(Engine& engine, IRender::Queue* queue) {
	// Force update source 
	RenderStage* renderStage = Front.linkedRenderStage;
	if (renderStage != nullptr) {
		IRender& render = engine.interfaces.render;
		IRender::Resource* next = Next.renderTargetTextureResource->GetTexture();
		IRender::Resource* front = Front.textureResource->GetTexture();

		if (Front.textureResource->description.state != Next.renderTargetTextureResource->description.state
			|| Front.textureResource->description.dimension.x() != Next.renderTargetTextureResource->description.dimension.x()
			|| Front.textureResource->description.dimension.y() != Next.renderTargetTextureResource->description.dimension.y()) {

			Next.renderTargetTextureResource->description.state = Front.textureResource->description.state;
			Next.renderTargetTextureResource->description.dimension.x() = Front.textureResource->description.dimension.x();
			Next.renderTargetTextureResource->description.dimension.y() = Front.textureResource->description.dimension.y();
			Next.renderTargetTextureResource->Flag().fetch_or(Tiny::TINY_MODIFIED, std::memory_order_release);
			Next.renderTargetTextureResource->GetResourceManager().InvokeUpload(Next.renderTargetTextureResource(), queue);
			// Front.textureResource->GetResourceManager().InvokeUpload(Front.textureResource(), queue);
		}

		render.SwapResource(queue, next, front);

		IRender::Resource::RenderTargetDescription copy = renderStage->renderTargetDescription;
		render.UploadResource(queue, renderStage->renderTarget, &copy);
	}
}

