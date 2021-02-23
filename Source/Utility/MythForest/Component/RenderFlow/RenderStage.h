// RenderStage.h
// PaintDream (paintdream@paintdream.com)
// 2018-9-11
//

#pragma once
#include "RenderPort.h"
#include "../../Engine.h"
#include "../../../SnowyStream/SnowyStream.h"
#include "../../../SnowyStream/Resource/MeshResource.h"
#include "../../../SnowyStream/Resource/MaterialResource.h"

namespace PaintsNow {
	class RenderFlowComponent;
	class RenderStage : public TReflected<RenderStage, GraphNode<SharedTiny, RenderPort> > {
	public:
		RenderStage(uint32_t colorAttachmentCount = 1);
		friend class RenderFlowComponent;

		enum {
			RENDERSTAGE_WEAK_LINKAGE = TINY_CUSTOM_BEGIN,
			RENDERSTAGE_ADAPT_MAIN_RESOLUTION = TINY_CUSTOM_BEGIN << 1,
			RENDERSTAGE_OUTPUT_TO_BACK_BUFFER = TINY_CUSTOM_BEGIN << 2,
			RENDERSTAGE_COMPUTE_PASS = TINY_CUSTOM_BEGIN << 3,
			RENDERSTAGE_CUSTOM_BEGIN = TINY_CUSTOM_BEGIN << 4
		};

		virtual void Initialize(Engine& engine, IRender::Queue* resourceQueue);
		virtual void Uninitialize(Engine& engine, IRender::Queue* resourceQueue);
		virtual void Prepare(Engine& engine, IRender::Queue* resourceQueue);
		virtual void Update(Engine& engine, IRender::Queue* resourceQueue);
		virtual void SetMainResolution(Engine& engine, IRender::Queue* resourceQueue, UShort2 res);
		virtual void Tick(Engine& engine, IRender::Queue* resourceQueue);
		virtual void Commit(Engine& engine, std::vector<IRender::Queue*>& queues, std::vector<IRender::Queue*>& instantQueues, std::vector<IRender::Queue*>& deletedQueues, IRender::Queue* instantQueue);

		uint16_t GetFrameBarrierIndex() const;
		void SetFrameBarrierIndex(uint16_t index);
		IRender::Resource* GetRenderTargetResource() const;
		const IRender::Resource::RenderTargetDescription& GetRenderTargetDescription() const;
		friend class FrameBarrierRenderStage;

		IRender::Resource::RenderStateDescription renderStateDescription;
		IRender::Resource::RenderTargetDescription renderTargetDescription;
		TShared<MaterialResource> overrideMaterial;

	protected:
		IRender::Resource* renderState;
		IRender::Resource* renderTarget;
		IRender::Resource* drawCallResource;
		Char2 resolutionShift;
		uint16_t frameBarrierIndex;
		std::vector<IRender::Resource*> newResources;
	};

	template <class T>
	class GeneralRenderStage : public TReflected<GeneralRenderStage<T>, RenderStage> {
	public:
		// gcc do not support referencing base type in template class. manunaly specified here.
		typedef TReflected<GeneralRenderStage<T>, RenderStage> BaseClass;
		GeneralRenderStage(uint32_t colorAttachmentCount = 1) : BaseClass(colorAttachmentCount) {}
		void Prepare(Engine& engine, IRender::Queue* queue) override {
			// create specified shader resource (if not exists)
			String path = ShaderResource::GetShaderPathPrefix() + UniqueType<T>::Get()->GetBriefName();
			sharedShader = engine.snowyStream.CreateReflectedResource(UniqueType<ShaderResource>(), path, true, ResourceBase::RESOURCE_VIRTUAL);
			shaderInstance.Reset(static_cast<ShaderResourceImpl<T>*>(sharedShader->Clone()));

			// Process compute shader stage.
			if (GetPass().ExportShaderStageMask() & (1 << IRender::Resource::ShaderDescription::COMPUTE)) {
				BaseClass::Flag().fetch_or(BaseClass::RENDERSTAGE_COMPUTE_PASS, std::memory_order_relaxed);
			}

			BaseClass::Prepare(engine, queue);
		}

	protected:
		inline IRender::Resource* GetShaderResource() const {
			IRender::Resource* resource = shaderInstance->GetShaderResource();
			return resource == nullptr ? sharedShader->GetShaderResource() : resource;
		}

		inline T& GetPass() {
			return static_cast<T&>(shaderInstance->GetPass());
		}

		PassBase::Updater& GetPassUpdater() {
			return shaderInstance->GetPassUpdater();
		}

		void Update(Engine& engine, IRender::Queue* queue) override {
			if (BaseClass::overrideMaterial && (BaseClass::overrideMaterial->Flag().load(std::memory_order_acquire) & Tiny::TINY_MODIFIED)) {
				if (BaseClass::overrideMaterial->Flag().fetch_and(~Tiny::TINY_MODIFIED) & Tiny::TINY_MODIFIED) {
					// flush material 
					BaseClass::overrideMaterial->Refresh(engine.interfaces.render, queue, shaderInstance());
				}
			}

			BaseClass::Update(engine, queue);
		}

	protected:
		TShared<ShaderResourceImpl<T> > shaderInstance;
		TShared<ShaderResource> sharedShader;
	};

	template <class T>
	class GeneralRenderStageDraw : public TReflected<GeneralRenderStageDraw<T>, GeneralRenderStage<T> > {
	public:
		// gcc do not support referencing base type in template class. manunaly specified here.
		typedef TReflected<GeneralRenderStageDraw<T>, GeneralRenderStage<T> > BaseClass;
		GeneralRenderStageDraw(uint32_t colorAttachmentCount = 1) : BaseClass(colorAttachmentCount) {}

		void Prepare(Engine& engine, IRender::Queue* queue) override {
			// create specified shader resource (if not exists)
			if (!(BaseClass::Flag().load(std::memory_order_relaxed) & BaseClass::RENDERSTAGE_COMPUTE_PASS)) {
				// prepare mesh
				if (!meshResource) {
					const String path = "[Runtime]/MeshResource/StandardQuad";
					meshResource = engine.snowyStream.CreateReflectedResource(UniqueType<MeshResource>(), path, true, ResourceBase::RESOURCE_VIRTUAL);
					assert(meshResource);
				}

				assert(meshResource->Flag().load(std::memory_order_acquire) & ResourceBase::RESOURCE_UPLOADED);
			}

			BaseClass::Prepare(engine, queue);
		}

		// Helper functions
		void Update(Engine& engine, IRender::Queue* queue) override {
			BaseClass::Update(engine, queue);

			IRender& render = engine.interfaces.render;
			PassBase::Updater& updater = BaseClass::GetPassUpdater();
			std::vector<Bytes> bufferData;
			updater.Capture(drawCallDescription, bufferData, 1 << IRender::Resource::BufferDescription::UNIFORM);
			// first time?
			if (BaseClass::drawCallResource == nullptr) {
				assert(BaseClass::newResources.empty());
				updater.Update(render, queue, drawCallDescription, BaseClass::newResources, bufferData,
					(1 << IRender::Resource::BufferDescription::VERTEX) |
					(1 << IRender::Resource::BufferDescription::UNIFORM) |
					(1 << IRender::Resource::BufferDescription::STORAGE) |
					(1 << IRender::Resource::BufferDescription::INSTANCED));
				drawCallDescription.shaderResource = BaseClass::GetShaderResource();
				BaseClass::drawCallResource = render.CreateResource(render.GetQueueDevice(queue), IRender::Resource::RESOURCE_DRAWCALL);
				if (!(BaseClass::Flag().load(std::memory_order_relaxed) & BaseClass::RENDERSTAGE_COMPUTE_PASS)) {
					drawCallDescription.indexBufferResource.buffer = meshResource->bufferCollection.indexBuffer;
				}
			} else {
				// recapture all data (uniforms by default)
				size_t count = BaseClass::newResources.size();
				updater.Update(render, queue, drawCallDescription, BaseClass::newResources, bufferData, 1 << IRender::Resource::BufferDescription::UNIFORM);
				assert(count == BaseClass::newResources.size()); // must not adding new resource(s)
			}

			IRender::Resource::DrawCallDescription copy = drawCallDescription;
			render.UploadResource(queue, BaseClass::drawCallResource, &copy);
		}

		void Commit(Engine& engine, std::vector<IRender::Queue*>& queues, std::vector<IRender::Queue*>& instantQueues, std::vector<IRender::Queue*>& deletedQueues, IRender::Queue* instantQueue) override {
			BaseClass::Commit(engine, queues, instantQueues, deletedQueues, instantQueue);
			IRender& render = engine.interfaces.render;
			render.ExecuteResource(instantQueue, BaseClass::drawCallResource);
		}

	protected:
		TShared<MeshResource> meshResource;
		IRender::Resource::DrawCallDescription drawCallDescription;
	};
}

