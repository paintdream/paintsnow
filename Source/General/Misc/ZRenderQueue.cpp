#include "ZRenderQueue.h"

using namespace PaintsNow;

ZRenderQueue::ZRenderQueue() : queue(nullptr) {
	invokeCounter.store(0, std::memory_order_relaxed);
}

void ZRenderQueue::Initialize(IRender& render, IRender::Device* device) {
	assert(queue == nullptr);
	queue = render.CreateQueue(device);
	render.YieldQueue(queue); // push a barrier first
}

void ZRenderQueue::Uninitialize(IRender& render) {
	assert(queue != nullptr);
	render.DeleteQueue(queue);
	queue = nullptr;
}

void ZRenderQueue::InvokeRenderQueuesParallel(IRender& render, std::vector<ZRenderQueue*>& queues) {
	if (queues.empty()) return;

	// cleanup stage
	while (true) {
		std::vector<IRender::Queue*> cleanupQueues;
		for (size_t i = 0; i < queues.size(); i++) {
			ZRenderQueue* q = queues[i];
			if (q->invokeCounter.load(std::memory_order_acquire) != 0) {
				cleanupQueues.emplace_back(q->queue);
				--q->invokeCounter;
			}
		}

		if (cleanupQueues.empty()) break;

		render.PresentQueues(&cleanupQueues[0], (uint32_t)cleanupQueues.size(), IRender::CLEANUP);
		cleanupQueues.clear();
	}

	std::vector<IRender::Queue*> renderQueues;
	for (size_t i = 0; i < queues.size(); i++) {
		renderQueues.emplace_back(queues[i]->queue);
	}

	render.PresentQueues(&renderQueues[0], (uint32_t)renderQueues.size(), IRender::REPEAT);
}

void ZRenderQueue::InvokeRender(IRender& render) {
	assert(queue != nullptr);
	while (invokeCounter.load(std::memory_order_acquire) != 0) {
		render.PresentQueues(&queue, 1, IRender::CLEANUP);
		--invokeCounter;
	}

	render.PresentQueues(&queue, 1, IRender::REPEAT);
}

void ZRenderQueue::UpdateFrame(IRender& render) {
	render.YieldQueue(queue);
	++invokeCounter;
}

IRender::Queue* ZRenderQueue::GetQueue() const {
	return queue;
}

bool ZRenderQueue::WaitUpdate() const {
	return invokeCounter.load(std::memory_order_relaxed) == 0;
}