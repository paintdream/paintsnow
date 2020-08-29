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
		virtual ~RenderPortCommandQueue();
		virtual TObject<IReflect>& operator () (IReflect& reflect) override;
		virtual void Initialize(IRender& render, IRender::Queue* mainQueue) override;
		virtual void Uninitialize(IRender& render, IRender::Queue* mainQueue) override;
		virtual void Commit(std::vector<IRender::Queue*>& fencedQueues, std::vector<IRender::Queue*>& instanceQueues) override;
		virtual bool UpdateDataStream(RenderPort& source) override;

		virtual bool BeginFrame(IRender& render);
		virtual void EndFrame(IRender& render);
		void MergeQueue(IRender& render, IRender::Queue* instanceQueue);
		void DrawElement(IRender& render, IRender::Resource* drawCallResource);
		void CheckinState(IRender& render, IRender::Resource* stateResource);

	protected:
		TQueueList<IRender::Queue*> mergedQueues;
		IRender::Queue* renderQueue;
	};
}

