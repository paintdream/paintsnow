#include "RenderStage.h"
#include "RenderPort/RenderPortRenderTarget.h"

using namespace PaintsNow;

RenderStage::RenderStage(uint32_t colorAttachmentCount) : renderState(nullptr), renderTarget(nullptr), drawCallResource(nullptr), resolutionShift(0, 0), frameBarrierIndex(0) {
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
	t.depthStorage.storeOp = IRender::Resource::RenderTargetDescription::DEFAULT;
}

uint16_t RenderStage::GetFrameBarrierIndex() const {
	return frameBarrierIndex;
}

void RenderStage::SetFrameBarrierIndex(uint16_t index) {
	frameBarrierIndex = index;
}

void RenderStage::Prepare(Engine& engine, IRender::Queue* queue) {
	IRender& render = engine.interfaces.render;
	IRender::Device* device = engine.snowyStream.GetRenderDevice();

	if (!(Flag().load(std::memory_order_relaxed) & RENDERSTAGE_COMPUTE_PASS)) {
		assert(renderState == nullptr);
		assert(renderTarget == nullptr);
		assert(!renderTargetDescription.colorStorages.empty());

		renderState = render.CreateResource(render.GetQueueDevice(queue), IRender::Resource::RESOURCE_RENDERSTATE);
		render.UploadResource(queue, renderState, &renderStateDescription);
		renderTarget = render.CreateResource(render.GetQueueDevice(queue), IRender::Resource::RESOURCE_RENDERTARGET);
	}
}

void RenderStage::Update(Engine& engine, IRender::Queue* queue) {
	Tiny::FLAG flag = Flag().load(std::memory_order_acquire);
	if (flag & RENDERSTAGE_COMPUTE_PASS) return; // no render targets

	IRender::Resource::RenderTargetDescription desc = renderTargetDescription;
	if (flag & RENDERSTAGE_OUTPUT_TO_BACK_BUFFER) {
		desc.colorStorages.resize(1);
		desc.colorStorages[0].backBuffer = 1;
		desc.colorStorages[0].resource = nullptr;
	} else {
		// optimize for Don't Care (DISCARD)
		const std::vector<PortInfo>& ports = GetPorts();
		for (size_t i = 0; i < ports.size(); i++) {
			RenderPortRenderTargetStore* store = ports[i].port->QueryInterface(UniqueType<RenderPortRenderTargetStore>());
			// Not referenced by any other nodes
			if (store != nullptr && store->GetLinks().empty()) {
				assert(store->attachedTexture);

				// can be safety discarded?
				if (store->attachedTexture->description.frameBarrierIndex < frameBarrierIndex) {
					if (&renderTargetDescription.depthStorage == &store->bindingStorage) {
						desc.depthStorage.storeOp = IRender::Resource::RenderTargetDescription::DISCARD;
					} else if (&renderTargetDescription.stencilStorage == &store->bindingStorage) {
						desc.stencilStorage.storeOp = IRender::Resource::RenderTargetDescription::DISCARD;
					} else {
						size_t index = &store->bindingStorage - &renderTargetDescription.colorStorages[0];
						desc.colorStorages[index].storeOp = IRender::Resource::RenderTargetDescription::DISCARD;
					}
				}
			}

			RenderPortRenderTargetLoad* load = ports[i].port->QueryInterface(UniqueType<RenderPortRenderTargetLoad>());
			if (load != nullptr && !load->GetLinks().empty()) {
				assert(load->GetLinks().size() == 1);
				RenderPortRenderTargetStore* upstream = static_cast<RenderPortRenderTargetStore*>(load->GetLinks().back().port);
				assert(upstream != nullptr);
				if (upstream->bindingStorage.storeOp == IRender::Resource::RenderTargetDescription::DISCARD) {
					assert(load->bindingStorage.loadOp != IRender::Resource::RenderTargetDescription::DEFAULT);
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
		Update(engine, queue);
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
	Tiny::FLAG flag = Flag().load(std::memory_order_relaxed);
	assert(Flag().load(std::memory_order_acquire) & TINY_ACTIVATED);
	for (size_t i = 0; i < nodePorts.size(); i++) {
		nodePorts[i].port->Commit(queues, instantQueues, deletedQueues);
	}

	if (!(flag & RENDERSTAGE_COMPUTE_PASS)) {
		IRender& render = engine.interfaces.render;
		render.ExecuteResource(instantQueue, renderState);
		render.ExecuteResource(instantQueue, renderTarget);
	}
}

void RenderStage::SetMainResolution(Engine& engine, IRender::Queue* resourceQueue, UShort2 res) {
	if (!(Flag().load(std::memory_order_acquire) & RENDERSTAGE_ADAPT_MAIN_RESOLUTION)) return;

	if (!(flag & RENDERSTAGE_COMPUTE_PASS)) {
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
	}

	Flag().fetch_or(TINY_MODIFIED, std::memory_order_relaxed);
}