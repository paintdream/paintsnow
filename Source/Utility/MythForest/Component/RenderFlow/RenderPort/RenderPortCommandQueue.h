// RenderPortCommandQueue.h
// PaintDream (paintdream@paintdream.com)
// 2018-10-15
//

#pragma once
#include "../RenderPort.h"
#include "../../Renderable/RenderableComponent.h"

namespace PaintsNow {
	class BatchComponent;
	class RenderPortCommandQueue : public TReflected<RenderPortCommandQueue, RenderPort> {
	public:
		RenderPortCommandQueue();
		~RenderPortCommandQueue() override;
		TObject<IReflect>& operator () (IReflect& reflect) override;
		void Initialize(IRender& render, IRender::Queue* mainQueue) override;
		void Uninitialize(IRender& render, IRender::Queue* mainQueue) override;
		void Commit(std::vector<IRender::Queue*>& fencedQueues, std::vector<IRender::Queue*>& instanceQueues, std::vector<IRender::Queue*>& deletedQueues) override;

		bool BeginFrame(IRender& render) override;
		void EndFrame(IRender& render) override;
		void MergeQueue(IRender& render, IRender::Queue* instanceQueue);
		void DeleteMergedQueue(IRender& render, IRender::Queue* mergedQueue);
		void DrawElement(IRender& render, IRender::Resource* drawCallResource);
		void CheckinState(IRender& render, IRender::Resource* stateResource);

	protected:
		TQueueList<IRender::Queue*> mergedQueues;
		IRender::Queue* renderQueue;
	};
}

