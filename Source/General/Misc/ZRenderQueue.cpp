#include "ZRenderQueue.h"

using namespace PaintsNow;

ZRenderQueue::ZRenderQueue() : queue(nullptr) {
	invokeCounter.store(0, std::memory_order_relaxed);
}

ZRenderQueue::~ZRenderQueue() {
	assert(queue == nullptr);
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

void ZRenderQueue::InvokeRenderQueuesParallel(IRender& render, std::vector<ZRenderQueue*>& queues, IRender::PresentOption option) {
	if (queues.empty()) return;

	// cleanup stage
	std::vector<IRender::Queue*> renderQueues;
	while (true) {
		for (size_t i = 0; i < queues.size(); i++) {
			ZRenderQueue* q = queues[i];
			if (q->invokeCounter.load(std::memory_order_acquire) > 1) {
				renderQueues.emplace_back(q->queue);
				q->invokeCounter.fetch_sub(1, std::memory_order_relaxed);
			}
		}

		if (renderQueues.empty()) break;

		render.PresentQueues(&renderQueues[0], (uint32_t)renderQueues.size(), IRender::UPDATE);
		renderQueues.clear();
	}

	for (size_t i = 0; i < queues.size(); i++) {
		ZRenderQueue* q = queues[i];
		if (q->invokeCounter.load(std::memory_order_acquire) != 0) {
			renderQueues.emplace_back(q->GetQueue());
			q->invokeCounter.fetch_sub(1, std::memory_order_relaxed);
		}
	}

	if (!renderQueues.empty()) {
		render.PresentQueues(&renderQueues[0], (uint32_t)renderQueues.size(), option);
	}
}

void ZRenderQueue::InvokeRender(IRender& render, IRender::PresentOption option) {
	assert(queue != nullptr);
	while (invokeCounter.load(std::memory_order_acquire) > 1) {
		render.PresentQueues(&queue, 1, IRender::UPDATE);
		invokeCounter.fetch_sub(1, std::memory_order_relaxed);
	}

	if (invokeCounter.load(std::memory_order_relaxed) != 0) {
		render.PresentQueues(&queue, 1, option);
		invokeCounter.fetch_sub(1, std::memory_order_relaxed);
	}
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