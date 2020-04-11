#include "RenderStage.h"
#include "RenderPort/RenderPortRenderTarget.h"
#include "RenderPort/RenderPortLoadTarget.h"

using namespace PaintsNow;
using namespace PaintsNow::NsMythForest;

RenderStage::RenderStage(uint32_t colorAttachmentCount) : renderState(nullptr), renderTarget(nullptr), clear(nullptr), drawCallResource(nullptr) {
	Flag() |= RENDERSTAGE_ADAPT_MAIN_RESOLUTION;

	// Initialize state
	IRender::Resource::RenderStateDescription& s = renderStateDescription;
	s.pass = 0;
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
	t.isBackBuffer = false;
	t.width = 0; // Invalid by default
	t.height = 0;
	t.colorBufferStorages.resize(colorAttachmentCount);
}

void RenderStage::PrepareResources(Engine& engine) {
	IRender& render = engine.interfaces.render;
	IRender::Device* device = engine.snowyStream.GetRenderDevice();
	assert(renderQueue.GetQueue() == nullptr);
	renderQueue.Initialize(render, device);

	IRender::Queue* queue = renderQueue.GetQueue();

	assert(renderState == nullptr);
	assert(renderTarget == nullptr);
	assert(!renderTargetDescription.colorBufferStorages.empty());

	renderState = render.CreateResource(queue, IRender::Resource::RESOURCE_RENDERSTATE);
	render.UploadResource(queue, renderState, &renderStateDescription);
	clear = render.CreateResource(queue, IRender::Resource::RESOURCE_CLEAR);
	render.UploadResource(queue, clear, &clearDescription);
	renderTarget = render.CreateResource(queue, IRender::Resource::RESOURCE_RENDERTARGET);

	// would be uploaded soon
	// IRender::Resource::RenderTargetDescription copy = renderTargetDescription;
	// render.UploadResource(queue, renderTarget, &copy);
}


class AutoAdaptRenderTarget : public IReflect {
public:
	AutoAdaptRenderTarget(IRender& r, IRender::Queue* q, uint32_t w, uint32_t h, bool sizeOnly) : IReflect(true, false), render(r), queue(q), width(w), height(h), updateSizeOnly(sizeOnly) {}

	virtual void Property(IReflectObject& s, Unique typeID, Unique refTypeID, const char* name, void* base, void* ptr, const MetaChainBase* meta) {
		static Unique unique = UniqueType<RenderPortRenderTarget>::Get();
		if (typeID == unique) {
			RenderPortRenderTarget& rt = static_cast<RenderPortRenderTarget&>(s);
			if (rt.renderTargetTextureResource) {
				UShort3& dimension = rt.renderTargetTextureResource->description.dimension;
				if (dimension.x() != width || dimension.y() != height || rt.bindingStorage.resource == nullptr) {
					dimension.x() = width;
					dimension.y() = height;

					if (!updateSizeOnly) {
						rt.bindingStorage.resource = rt.renderTargetTextureResource->GetTexture();

						// Update texture
						IRender::Resource::TextureDescription desc = rt.renderTargetTextureResource->description;
						render.UploadResource(queue, rt.renderTargetTextureResource->instance, &desc);
					}
				}
			}
		}
	}

	virtual void Method(Unique typeID, const char* name, const TProxy<>* p, const Param& retValue, const std::vector<Param>& params, const MetaChainBase* meta) {}

	IRender& render;
	IRender::Queue* queue;
	uint32_t width;
	uint32_t height;
	bool updateSizeOnly;
};

void RenderStage::UpdateRenderTarget(Engine& engine, IRender::Queue* resourceQueue, bool updateSizeOnly) {
	if (!(Flag() & RENDERSTAGE_ADAPT_MAIN_RESOLUTION)) return;

	// by now we have no color-free render buffers
	assert(!renderTargetDescription.colorBufferStorages.empty());
	IRender& render = engine.interfaces.render;
	
	uint16_t width = mainResolution.x();
	uint16_t height = mainResolution.y();
	assert(width != 0 && height != 0);
	renderTargetDescription.width = resolutionShift.x() > 0 ? Max(width >> resolutionShift.x(), 2) : width << resolutionShift.x();
	renderTargetDescription.height = resolutionShift.y() > 0 ? Max(height >> resolutionShift.y(), 2) : height << resolutionShift.y();

	AutoAdaptRenderTarget adapt(render, resourceQueue, renderTargetDescription.width, renderTargetDescription.height, updateSizeOnly);
	(*this)(adapt);

	if (!updateSizeOnly) {
		// Commit
		assert(renderTarget != nullptr);
		IRender::Resource::RenderTargetDescription copy = renderTargetDescription;
		render.UploadResource(resourceQueue, renderTarget, &copy);
	}
}

void RenderStage::UpdatePass(Engine& engine) {
	IRender& render = engine.interfaces.render;
	IRender::Queue* queue = renderQueue.GetQueue();
	render.ExecuteResource(queue, renderTarget);
	render.ExecuteResource(queue, renderState);
	render.ExecuteResource(queue, clear);
}

void RenderStage::UpdateComplete(Engine& engine) {
	IRender& render = engine.interfaces.render;
	renderQueue.UpdateFrame(render);
}

void RenderStage::Initialize(Engine& engine) {
	IRender& render = engine.interfaces.render;
	IRender::Queue* queue = renderQueue.GetQueue();
	for (size_t i = 0; i < nodePorts.size(); i++) {
		Port* port = nodePorts[i].port;
		for (size_t j = 0; j < port->GetLinks().size(); j++) {
			if (!(port->GetLinks()[j].flag & Tiny::TINY_PINNED)) {
				port->UpdateDataStream(*static_cast<Port*>(port->GetLinks()[j].port));
			}
		}

		port->Initialize(render, queue);
	}

	UpdateRenderTarget(engine, queue, false);
	UpdatePass(engine);
	UpdateComplete(engine);

	Flag() |= TINY_ACTIVATED;
}

void RenderStage::Uninitialize(Engine& engine) {
	Flag() &= ~TINY_ACTIVATED;

	IRender& render = engine.interfaces.render;
	IRender::Queue* queue = renderQueue.GetQueue();
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

	renderQueue.UpdateFrame(render);
	renderQueue.Uninitialize(render);
}

void RenderStage::Tick(Engine& engine) {
	Tiny::FLAG flag = Flag().load(std::memory_order_relaxed);
	for (size_t i = 0; i < nodePorts.size(); i++) {
		nodePorts[i].port->Tick(engine);
		flag |= nodePorts[i].port->Flag().load(std::memory_order_relaxed);
	}

	if (flag & TINY_MODIFIED) {
		UpdatePass(engine);
		UpdateComplete(engine);
		Flag() &= ~TINY_MODIFIED;
	}
}

IRender::Resource* RenderStage::GetRenderTargetResource() const {
	return renderTarget;
}

const IRender::Resource::RenderTargetDescription& RenderStage::GetRenderTargetDescription() const {
	return renderTargetDescription;
}

void RenderStage::PrepareRenderQueues(Engine& engine, std::vector<ZRenderQueue*>& queues) {
	assert(Flag() & TINY_ACTIVATED);

	queues.emplace_back(&renderQueue);
	IRender::Queue* queue = renderQueue.GetQueue();
	for (size_t i = 0; i < nodePorts.size(); i++) {
		nodePorts[i].port->PrepareRenderQueues(queues);
	}
}

void RenderStage::SetMainResolution(Engine& engine, IRender::Queue* resourceQueue, uint32_t width, uint32_t height, bool updateSizeOnly) {
	// By default, create render buffer with resolution provided
	// For some stages(e.g. cascaded bloom generator), we must override this function to adapt the new value

	mainResolution = UShort2(width, height);

	UpdateRenderTarget(engine, resourceQueue, updateSizeOnly);
	Flag() |= TINY_MODIFIED;
}