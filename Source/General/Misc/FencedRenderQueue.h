// FencedRenderQueue.h
// PaintDream (paintdream@paintdream.com)
// 2019-1-7
//

#ifndef __FENCEDRENDERQUEUE_H__
#define __FENCEDRENDERQUEUE_H__

#include "../../General/Interface/IRender.h"
#include "../../Core/Template/TAtomic.h"

namespace PaintsNow {
	// repeatable render queue
	class FencedRenderQueue {
	public:
		FencedRenderQueue();
		~FencedRenderQueue();
		void Initialize(IRender& render, IRender::Device* device);
		void Uninitialize(IRender& render);
		void InvokeRender(IRender& render);
		static void InvokeRenderQueuesParallel(IRender& render, std::vector<FencedRenderQueue*>& queues);
		void UpdateFrame(IRender& render);
		IRender::Queue* GetQueue() const;
		bool WaitUpdate() const;

	protected:
		IRender::Queue* queue;
		std::atomic<int32_t> yieldCount;
	};
}

#endif // __FENCEDRENDERQUEUE_H__