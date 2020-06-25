// ZFencedRenderQueue.h
// PaintDream (paintdream@paintdream.com)
// 2019-1-7
//

#ifndef __ZFENCEDRENDERQUEUE_H__
#define __ZFENCEDRENDERQUEUE_H__

#include "../../General/Interface/IRender.h"
#include "../../Core/Template/TAtomic.h"

namespace PaintsNow {
	// repeatable render queue
	class ZFencedRenderQueue {
	public:
		ZFencedRenderQueue();
		~ZFencedRenderQueue();
		void Initialize(IRender& render, IRender::Device* device);
		void Uninitialize(IRender& render);
		void InvokeRender(IRender& render);
		static void InvokeRenderQueuesParallel(IRender& render, std::vector<ZFencedRenderQueue*>& queues);
		void UpdateFrame(IRender& render);
		IRender::Queue* GetQueue() const;
		bool WaitUpdate() const;

	protected:
		IRender::Queue* queue;
		std::atomic<int32_t> yieldCount;
	};
}

#endif // __ZFENCEDRENDERQUEUE_H__