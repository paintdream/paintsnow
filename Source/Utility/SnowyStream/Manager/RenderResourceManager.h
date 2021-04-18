// RenderResourceManager.h
// PaintDream (paintdreawm@paintdream.com)
// 2020-04-18
//

#pragma once

#include "../ResourceManager.h"
#include "../../../General/Interface/IRender.h"

namespace PaintsNow {
	// Specification for IRender
	class RenderResourceManager : public DeviceResourceManager<IRender> {
	public:
		typedef DeviceResourceManager<IRender> BaseClass;
		RenderResourceManager(ThreadPool& tp, IUniformResourceManager& hostManager, IRender& dev, const TWrapper<void, const String&>& errorHandler, void* context);
		~RenderResourceManager() override;

		IRender::Device* GetRenderDevice() const;
		IRender::Queue* GetResourceQueue();
		size_t NotifyCompletion(const TShared<ResourceBase>& resource);
		uint32_t GetRenderResourceFrameStep() const;
		void TickDevice(IDevice& device) override;
		size_t GetProfile(const String& feature);
		void SetRenderResourceFrameStep(uint32_t limitStep);
		size_t GetCurrentRuntimeVersion() const;
		size_t GetNextRuntimeVersion() const;
		bool GetCompleted() const;

		TObject<IReflect>& operator () (IReflect& reflect) override;

	protected:
		void RegisterBuiltinPasses();
		void RegisterBuiltinResources();
		void CreateBuiltinSolidTexture(const String& path, const UChar4& color);
		void CreateBuiltinMesh(const String& path, const Float3* vertices, size_t vertexCount, const UInt3* indices, size_t indexCount);

	protected:
		// Render
		IRender::Device* renderDevice;
		IRender::Queue* resourceQueue;
		uint32_t renderResourceStepPerFrame;
		std::atomic<uint32_t> currentNotifiedResourceCount;
		std::atomic<size_t> currentRuntimeVersion;
		std::atomic<size_t> nextRuntimeVersion;
		TQueueList<TShared<ResourceBase> > pendingCompletionResources;
	};
}
