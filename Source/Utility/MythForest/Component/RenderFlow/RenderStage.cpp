#include "RenderStage.h"
#include "RenderPort/RenderPortRenderTarget.h"
#include "RenderPort/RenderPortLoadTarget.h"

using namespace PaintsNow;
using namespace PaintsNow::NsMythForest;
using namespace PaintsNow::NsSnowyStream;

RenderStage::RenderStage(uint32_t colorAttachmentCount) : renderState(nullptr), renderTarget(nullptr), clear(nullptr), drawCallResource(nullptr), resolutionShift(0, 0) {
	Flag().fetch_or(RENDERSTAGE_ADAPT_MAIN_RESOLUTION, std::memory_order_acquire);

	// Initialize state
	IRender::Resource::RenderStateDescription& s = renderStateDescription;
	s.stencilReplacePass = 1;
	s.cull = 1;
	s.fill = 1;
	s.alphaBlend = 0;
	s.colorWrite = 1;
	s.depthTest = 0;
	s.depthWrite = 0;
	s.stencilTest = 0;
	s.stencilWrite = 0;
	s.stencilValue = 0;
	s.stencilMask = 0;

	IRender::Resource::ClearDescription& c = clearDescription;
	// do clear setting ...
	uint8_t clearMask = IRender::Resource::ClearDescription::DISCARD_LOAD | IRender::Resource::ClearDescription::DISCARD_STORE;
	c.clearColorBit = IRender::Resource::ClearDescription::DISCARD_LOAD;
	c.clearDepthBit = clearMask;
	c.clearStencilBit = clearMask;

	IRender::Resource::RenderTargetDescription& t = renderTargetDescription;
	t.colorBufferStorages.resize(colorAttachmentCount);
}

void RenderStage::PrepareResources(Engine& engine, IRender::Queue* queue) {
	IRender& render = engine.interfaces.render;
	IRender::Device* device = engine.snowyStream.GetRenderDevice();

	assert(renderState == nullptr);
	assert(renderTarget == nullptr);
	assert(!renderTargetDescription.colorBufferStorages.empty());

	renderState = render.CreateResource(render.GetQueueDevice(queue), IRender::Resource::RESOURCE_RENDERSTATE);
	render.UploadResource(queue, renderState, &renderStateDescription);
	clear = render.CreateResource(render.GetQueueDevice(queue), IRender::Resource::RESOURCE_CLEAR);
	render.UploadResource(queue, clear, &clearDescription);
	renderTarget = render.CreateResource(render.GetQueueDevice(queue), IRender::Resource::RESOURCE_RENDERTARGET);
}


class AutoAdaptRenderTarget : public IReflect {
public:
	AutoAdaptRenderTarget(IRender& r, IRender::Queue* q, uint32_t w, uint32_t h) : IReflect(true, false), render(r), queue(q), width(w), height(h) {}

	virtual void Property(IReflectObject& s, Unique typeID, Unique refTypeID, const char* name, void* base, void* ptr, const MetaChainBase* meta) {
		static Unique unique = UniqueType<RenderPortRenderTarget>::Get();
		if (typeID == unique) {
			RenderPortRenderTarget& rt = static_cast<RenderPortRenderTarget&>(s);
			if (rt.renderTargetTextureResource) {
				UShort3& dimension = rt.renderTargetTextureResource->description.dimension;
				if (dimension.x() != width || dimension.y() != height || rt.bindingStorage.resource == nullptr) {
					dimension.x() = width;
					dimension.y() = height;

					rt.bindingStorage.resource = rt.renderTargetTextureResource->GetTexture();

					// Update texture
					rt.renderTargetTextureResource->Flag().fetch_or(Tiny::TINY_MODIFIED, std::memory_order_release);
					rt.renderTargetTextureResource->GetResourceManager().InvokeUpload(rt.renderTargetTextureResource(), queue);
				}
			}
		}
	}

	virtual void Method(Unique typeID, const char* name, const TProxy<>* p, const Param& retValue, const std::vector<Param>& params, const MetaChainBase* meta) {}

	IRender& render;
	IRender::Queue* queue;
	uint32_t width;
	uint32_t height;
};

void RenderStage::UpdatePass(Engine& engine, IRender::Queue* queue) {
}

void RenderStage::Initialize(Engine& engine, IRender::Queue* queue) {
	IRender& render = engine.interfaces.render;
	for (size_t i = 0; i < nodePorts.size(); i++) {
		Port* port = nodePorts[i].port;
		for (size_t j = 0; j < port->GetLinks().size(); j++) {
			if (!(port->GetLinks()[j].flag & Tiny::TINY_PINNED)) {
				port->UpdateDataStream(*static_cast<Port*>(port->GetLinks()[j].port));
			}
		}

		port->Initialize(render, queue);
	}

	UpdatePass(engine, queue);

	if (renderTarget != nullptr) {
		IRender::Resource::RenderTargetDescription copy = renderTargetDescription;
		render.UploadResource(queue, renderTarget, &copy);
	}

	Flag().fetch_or(TINY_ACTIVATED, std::memory_order_acquire);
}

void RenderStage::Uninitialize(Engine& engine, IRender::Queue* queue) {
	Flag().fetch_and(~TINY_ACTIVATED, std::memory_order_release);

	IRender& render = engine.interfaces.render;
	for (size_t k = 0; k < nodePorts.size(); k++) {
		nodePorts[k].port->Uninitialize(render, queue);
	}

	for (size_t i = 0; i < newResources.size(); i++) {
		render.DeleteResource(queue, newResources[i]);
	}

	newResources.clear();

	if (drawCallResource != nullptr) {
		render.DeleteResource(queue, drawCallResource);
	}

	if (renderState != nullptr) {
		render.DeleteResource(queue, renderState);
	}

	if (renderTarget != nullptr) {
		render.DeleteResource(queue, renderTarget);
	}

	if (clear != nullptr) {
		render.DeleteResource(queue, clear);
	}
}

void RenderStage::Tick(Engine& engine, IRender::Queue* queue) {
	Tiny::FLAG flag = Flag().load(std::memory_order_relaxed);
	for (size_t i = 0; i < nodePorts.size(); i++) {
		nodePorts[i].port->Tick(engine, queue);
		flag |= nodePorts[i].port->Flag().load(std::memory_order_relaxed);
	}

	if (flag & TINY_MODIFIED) {
		UpdatePass(engine, queue);
		Flag().fetch_and(~TINY_MODIFIED, std::memory_order_release);
	}
}

IRender::Resource* RenderStage::GetRenderTargetResource() const {
	return renderTarget;
}

const IRender::Resource::RenderTargetDescription& RenderStage::GetRenderTargetDescription() const {
	return renderTargetDescription;
}

void RenderStage::Commit(Engine& engine, std::vector<FencedRenderQueue*>& queues, IRender::Queue* instantQueue) {
	assert(Flag() & TINY_ACTIVATED);
	for (size_t i = 0; i < nodePorts.size(); i++) {
		nodePorts[i].port->Commit(queues);
	}

	IRender& render = engine.interfaces.render;
	render.ExecuteResource(instantQueue, renderTarget);
	render.ExecuteResource(instantQueue, renderState);
	render.ExecuteResource(instantQueue, clear);
}

void RenderStage::SetMainResolution(Engine& engine, IRender::Queue* resourceQueue, uint32_t width, uint32_t height) {
	if (!(Flag() & RENDERSTAGE_ADAPT_MAIN_RESOLUTION)) return;	
	// By default, create render buffer with resolution provided
	// For some stages(e.g. cascaded bloom generator), we must override this function to adapt the new value
	// by now we have no color-free render buffers
	IRender& render = engine.interfaces.render;
	assert(width != 0 && height != 0);
	width = resolutionShift.x() > 0 ? Math::Max(width >> resolutionShift.x(), 2u) : width << resolutionShift.x();
	height = resolutionShift.y() > 0 ? Math::Max(height >> resolutionShift.y(), 2u) : height << resolutionShift.y();

	AutoAdaptRenderTarget adapt(render, resourceQueue, width, height);
	(*this)(adapt);

	Flag().fetch_or(TINY_MODIFIED, std::memory_order_acquire);
}