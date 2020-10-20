#include "RenderStage.h"
#include "RenderPort/RenderPortRenderTarget.h"

using namespace PaintsNow;

RenderStage::RenderStage(uint32_t colorAttachmentCount) : renderState(nullptr), renderTarget(nullptr), drawCallResource(nullptr), resolutionShift(0, 0) {
	Flag().fetch_or(RENDERSTAGE_ADAPT_MAIN_RESOLUTION, std::memory_order_relaxed);

	// Initialize state
	IRender::Resource::RenderStateDescription& s = renderStateDescription;
	s.stencilReplacePass = 0;
	s.cull = 1;
	s.fill = 1;
	s.blend = 0;
	s.colorWrite = 1;
	s.depthTest = 0;
	s.depthWrite = 0;
	s.stencilTest = 0;
	s.stencilWrite = 0;
	s.stencilValue = 0;
	s.stencilMask = 0;

	IRender::Resource::RenderTargetDescription& t = renderTargetDescription;
	t.colorStorages.resize(colorAttachmentCount);
	for (size_t i = 0; i < t.colorStorages.size(); i++) {
		IRender::Resource::RenderTargetDescription::Storage& s = t.colorStorages[i];
		s.loadOp = IRender::Resource::RenderTargetDescription::DISCARD;
		s.storeOp = IRender::Resource::RenderTargetDescription::DEFAULT;
	}

	t.depthStorage.loadOp = IRender::Resource::RenderTargetDescription::DISCARD;
	t.depthStorage.storeOp = IRender::Resource::RenderTargetDescription::DISCARD;
}

void RenderStage::PrepareResources(Engine& engine, IRender::Queue* queue) {
	IRender& render = engine.interfaces.render;
	IRender::Device* device = engine.snowyStream.GetRenderDevice();

	assert(renderState == nullptr);
	assert(renderTarget == nullptr);
	assert(!renderTargetDescription.colorStorages.empty());

	renderState = render.CreateResource(render.GetQueueDevice(queue), IRender::Resource::RESOURCE_RENDERSTATE);
	render.UploadResource(queue, renderState, &renderStateDescription);
	renderTarget = render.CreateResource(render.GetQueueDevice(queue), IRender::Resource::RESOURCE_RENDERTARGET);
}

void RenderStage::UpdatePass(Engine& engine, IRender::Queue* queue) {
	IRender::Resource::RenderTargetDescription desc = renderTargetDescription;

	if (Flag() & RENDERSTAGE_OUTPUT_TO_BACK_BUFFER) {
		desc.colorStorages.clear();
	} else {
		// optimize for Don't Care (DISCARD)
		const std::vector<PortInfo>& ports = GetPorts();
		for (size_t i = 0; i < ports.size(); i++) {
			RenderPortRenderTargetStore* rt = ports[i].port->QueryInterface(UniqueType<RenderPortRenderTargetStore>());
			// Not referenced by any other nodes
			if (rt != nullptr && rt->GetLinks().empty()) {
				if (&renderTargetDescription.depthStorage == &rt->bindingStorage) {
					desc.depthStorage.storeOp = IRender::Resource::RenderTargetDescription::DISCARD;
				} else if (&renderTargetDescription.stencilStorage == &rt->bindingStorage) {
					desc.stencilStorage.storeOp = IRender::Resource::RenderTargetDescription::DISCARD;
				} else {
					size_t index = &rt->bindingStorage - &renderTargetDescription.colorStorages[0];
					desc.colorStorages[index].storeOp = IRender::Resource::RenderTargetDescription::DISCARD;
				}
			}
		}
	}
	
	engine.interfaces.render.UploadResource(queue, renderTarget, &desc);
}

void RenderStage::Initialize(Engine& engine, IRender::Queue* queue) {
	IRender& render = engine.interfaces.render;
	for (size_t i = 0; i < nodePorts.size(); i++) {
		nodePorts[i].port->Initialize(render, queue);
	}

	Flag().fetch_or(TINY_ACTIVATED, std::memory_order_relaxed);
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

void RenderStage::Commit(Engine& engine, std::vector<IRender::Queue*>& queues, std::vector<IRender::Queue*>& instantQueues, std::vector<IRender::Queue*>& deletedQueues, IRender::Queue* instantQueue) {
	assert(Flag() & TINY_ACTIVATED);
	for (size_t i = 0; i < nodePorts.size(); i++) {
		nodePorts[i].port->Commit(queues, instantQueues, deletedQueues);
	}

	IRender& render = engine.interfaces.render;
	render.ExecuteResource(instantQueue, renderState);
	render.ExecuteResource(instantQueue, renderTarget);
}

void RenderStage::SetMainResolution(Engine& engine, IRender::Queue* resourceQueue, UShort2 res) {
	if (!(Flag() & RENDERSTAGE_ADAPT_MAIN_RESOLUTION)) return;	
	// By default, create render buffer with resolution provided
	// For some stages(e.g. cascaded bloom generator), we must override this function to adapt the new value
	// by now we have no color-free render buffers
	uint16_t width = res.x(), height = res.y();
	IRender& render = engine.interfaces.render;
	assert(width != 0 && height != 0);
	width = safe_cast<uint16_t>(resolutionShift.x() > 0 ? Math::Max(width >> resolutionShift.x(), 2) : width << resolutionShift.x());
	height = safe_cast<uint16_t>(resolutionShift.y() > 0 ? Math::Max(height >> resolutionShift.y(), 2) : height << resolutionShift.y());

	const std::vector<PortInfo>& portInfos = GetPorts();
	for (size_t i = 0; i < portInfos.size(); i++) {
		RenderPortRenderTargetStore* rt = portInfos[i].port->QueryInterface(UniqueType<RenderPortRenderTargetStore>());
		if (rt != nullptr) {
			rt->renderTargetDescription.dimension.x() = width;
			rt->renderTargetDescription.dimension.y() = height;

			if (rt->attachedTexture) {
				rt->attachedTexture->description.dimension = rt->renderTargetDescription.dimension;
				render.UploadResource(resourceQueue, rt->attachedTexture->GetRenderResource(), &rt->attachedTexture->description);
			}
		}
	}

	Flag().fetch_or(TINY_MODIFIED, std::memory_order_relaxed);
}