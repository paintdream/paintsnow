#include "FrameBarrierRenderStage.h"

using namespace PaintsNow;

FrameBarrierRenderStage::FrameBarrierRenderStage(const String& s) : Next(renderTargetDescription.colorStorages[0]) {
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
	Next.renderTargetDescription.state.immutable = false;
	Next.renderTargetDescription.state.attachment = true;
	/*
	Next.renderTargetTextureResource->description.state.format = IRender::Resource::TextureDescription::UNSIGNED_BYTE;
	Next.renderTargetTextureResource->description.state.layout = IRender::Resource::TextureDescription::RGBA;
	*/
	// Copy descriptions from tracked render stage
	// RenderStage::PrepareResources(engine);
}

void FrameBarrierRenderStage::SetMainResolution(Engine& engine, IRender::Queue* queue, uint32_t width, uint32_t height) {}
void FrameBarrierRenderStage::UpdatePass(Engine& engine, IRender::Queue* queue) {}
void FrameBarrierRenderStage::Commit(Engine& engine, std::vector<IRender::Queue*>& queues, std::vector<IRender::Queue*>& instantQueues, std::vector<IRender::Queue*>& deletedQueues, IRender::Queue* instantQueue) {}

void FrameBarrierRenderStage::Tick(Engine& engine, IRender::Queue* queue) {
	// Force update source 
	RenderStage* renderStage = Front.Get;
	if (renderStage != nullptr) {
		IRender& render = engine.interfaces.render;
		IRender::Resource* next = Next.renderTargetDescription->GetRenderResource();
		IRender::Resource* front = Front.textureResource->GetRenderResource();

		if (Front.textureResource->description.state != Next.renderTargetDescription.state
			|| Front.textureResource->description.dimension.x() != Next.renderTargetDescription.dimension.x()
			|| Front.textureResource->description.dimension.y() != Next.renderTargetDescription.dimension.y()) {

			Next.renderTargetDescription.state = Front.textureResource->description.state;
			Next.renderTargetDescription.dimension.x() = Front.textureResource->description.dimension.x();
			Next.renderTargetDescription.dimension.y() = Front.textureResource->description.dimension.y();
			Next.renderTargetDescription->Flag().fetch_or(Tiny::TINY_MODIFIED, std::memory_order_release);
			Next.renderTargetDescription->GetResourceManager().InvokeUpload(Next.renderTargetDescription(), queue);
			// Front.textureResource->GetResourceManager().InvokeUpload(Front.textureResource(), queue);
		}
	}
}

