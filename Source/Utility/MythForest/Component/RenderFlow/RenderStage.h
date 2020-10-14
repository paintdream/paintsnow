// RenderStage.h
// PaintDream (paintdream@paintdream.com)
// 2018-9-11
//

#pragma once
#include "RenderPort.h"
#include "../../Engine.h"
#include "../../../SnowyStream/SnowyStream.h"
#include "../../../SnowyStream/Resource/MeshResource.h"

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
			RENDERSTAGE_CUSTOM_BEGIN = TINY_CUSTOM_BEGIN << 3
		};

		virtual void PrepareResources(Engine& engine, IRender::Queue* resourceQueue);
		virtual void Initialize(Engine& engine, IRender::Queue* resourceQueue);
		virtual void Uninitialize(Engine& engine, IRender::Queue* resourceQueue);
		virtual void SetMainResolution(Engine& engine, IRender::Queue* resourceQueue, UShort2 res);
		virtual void UpdatePass(Engine& engine, IRender::Queue* resourceQueue);
		virtual void Tick(Engine& engine, IRender::Queue* resourceQueue);
		virtual void Commit(Engine& engine, std::vector<IRender::Queue*>& queues, std::vector<IRender::Queue*>& instantQueues, std::vector<IRender::Queue*>& deletedQueues, IRender::Queue* instantQueue);

		IRender::Resource* GetRenderTargetResource() const;
		const IRender::Resource::RenderTargetDescription& GetRenderTargetDescription() const;
		friend class FrameBarrierRenderStage;

		IRender::Resource::RenderStateDescription renderStateDescription;
		IRender::Resource::RenderTargetDescription renderTargetDescription;

	protected:
		IRender::Resource* renderState;
		IRender::Resource* renderTarget;
		IRender::Resource* drawCallResource;
		Char2 resolutionShift;
		Char2 reserved;
		std::vector<IRender::Resource*> newResources;
	};

	template <class T>
	class GeneralRenderStage : public TReflected<GeneralRenderStage<T>, RenderStage> {
	public:
		// gcc do not support referencing base type in template class. manunaly specified here.
		typedef TReflected<GeneralRenderStage<T>, RenderStage> BaseClass;
		GeneralRenderStage(uint32_t colorAttachmentCount = 1) : BaseClass(colorAttachmentCount) {}
		void PrepareResources(Engine& engine, IRender::Queue* queue) override {
			// create specified shader resource (if not exists)
			String path = ShaderResource::GetShaderPathPrefix() + UniqueType<T>::Get()->GetBriefName();
			sharedShader = engine.snowyStream.CreateReflectedResource(UniqueType<ShaderResource>(), path, true, 0, nullptr);
			shaderInstance.Reset(static_cast<ShaderResourceImpl<T>*>(sharedShader->Clone()));
			BaseClass::PrepareResources(engine, queue);
		}

	protected:
		inline IRender::Resource* GetShaderResource() const {
			return sharedShader->GetShaderResource();
		}

		inline T& GetPass() {
			return static_cast<T&>(shaderInstance->GetPass());
		}

		PassBase::Updater& GetPassUpdater() {
			return shaderInstance->GetPassUpdater();
		}

	protected:
		TShared<ShaderResourceImpl<T> > shaderInstance;
		TShared<ShaderResource> sharedShader;
	};

	template <class T>
	class GeneralRenderStageMesh : public TReflected<GeneralRenderStageMesh<T>, GeneralRenderStage<T> > {
	public:
		// gcc do not support referencing base type in template class. manunaly specified here.
		typedef TReflected<GeneralRenderStageMesh<T>, GeneralRenderStage<T> > BaseClass;
		GeneralRenderStageMesh(uint32_t colorAttachmentCount = 1) : BaseClass(colorAttachmentCount) {}

		void PrepareResources(Engine& engine, IRender::Queue* queue) override {
			// create specified shader resource (if not exists)
			if (!meshResource) {
				const String path = "[Runtime]/MeshResource/StandardQuad";
				meshResource = engine.snowyStream.CreateReflectedResource(UniqueType<MeshResource>(), path, true, 0, nullptr);
			}

			assert(meshResource->Flag() & ResourceBase::RESOURCE_UPLOADED);
			BaseClass::PrepareResources(engine, queue);
		}

		// Helper functions
		void UpdatePass(Engine& engine, IRender::Queue* queue) override {
			BaseClass::UpdatePass(engine, queue);

			IRender& render = engine.interfaces.render;
			PassBase::Updater& updater = BaseClass::GetPassUpdater();
			std::vector<Bytes> bufferData;
			updater.Capture(drawCallDescription, bufferData, 1 << IRender::Resource::BufferDescription::UNIFORM);
			// first time?
			if (BaseClass::drawCallResource == nullptr) {
				assert(BaseClass::newResources.empty());
				updater.Update(render, queue, drawCallDescription, BaseClass::newResources, bufferData,
					(1 << IRender::Resource::BufferDescription::VERTEX) | (1 << IRender::Resource::BufferDescription::UNIFORM) | (1 << IRender::Resource::BufferDescription::INSTANCED));
				drawCallDescription.indexBufferResource.buffer = meshResource->bufferCollection.indexBuffer;
				drawCallDescription.shaderResource = BaseClass::GetShaderResource();
				BaseClass::drawCallResource = render.CreateResource(render.GetQueueDevice(queue), IRender::Resource::RESOURCE_DRAWCALL);
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

