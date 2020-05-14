// ZRenderQueue.h
// PaintDream (paintdream@paintdream.com)
// 2019-1-7
//

#ifndef __ZRENDERQUEUE_H__
#define __ZRENDERQUEUE_H__

#include "../../General/Interface/IRender.h"
#include "../../Core/Template/TAtomic.h"

namespace PaintsNow {
	// repeatable render queue
	class ZRenderQueue {
	public:
		ZRenderQueue();
		~ZRenderQueue();
		void Initialize(IRender& render, IRender::Device* device);
		void Uninitialize(IRender& render);
		void InvokeRender(IRender& render, IRender::PresentOption option);
		static void InvokeRenderQueuesParallel(IRender& render, std::vector<ZRenderQueue*>& queues, IRender::PresentOption option);
		void UpdateFrame(IRender& render);
		IRender::Queue* GetQueue() const;
		bool WaitUpdate() const;

	protected:
		IRender::Queue* queue;
		TAtomic<int32_t> invokeCounter;
	};
}

#endif // __ZRENDERQUEUE_H__