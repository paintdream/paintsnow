#include "ZFencedRenderQueue.h"

using namespace PaintsNow;

ZFencedRenderQueue::ZFencedRenderQueue() : queue(nullptr) {
	invokeCounter.store(0, std::memory_order_relaxed);
}

ZFencedRenderQueue::~ZFencedRenderQueue() {
	assert(queue == nullptr);
}

void ZFencedRenderQueue::Initialize(IRender& render, IRender::Device* device) {
	assert(queue == nullptr);
	queue = render.CreateQueue(device);
}

void ZFencedRenderQueue::Uninitialize(IRender& render) {
	assert(queue != nullptr);
	render.DeleteQueue(queue);
	queue = nullptr;
}

void ZFencedRenderQueue::InvokeRenderQueuesParallel(IRender& render, std::vector<ZFencedRenderQueue*>& queues, IRender::PresentOption option) {
	if (queues.empty()) return;

	// cleanup stage
	std::vector<IRender::Queue*> renderQueues;
	while (true) {
		for (size_t i = 0; i < queues.size(); i++) {
			ZFencedRenderQueue* q = queues[i];
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
		ZFencedRenderQueue* q = queues[i];
		if (q->invokeCounter.load(std::memory_order_acquire) != 0) {
			renderQueues.emplace_back(q->GetQueue());
		}
	}

	if (!renderQueues.empty()) {
		render.PresentQueues(&renderQueues[0], (uint32_t)renderQueues.size(), option);
	}
}

void ZFencedRenderQueue::InvokeRender(IRender& render, IRender::PresentOption option) {
	assert(queue != nullptr);
	if (invokeCounter.load(std::memory_order_relaxed) > 0) {
		while (invokeCounter.load(std::memory_order_acquire) > 1) {
			render.PresentQueues(&queue, 1, IRender::UPDATE);
			invokeCounter.fetch_sub(1, std::memory_order_relaxed);
		}

		render.PresentQueues(&queue, 1, option);
	}
}

void ZFencedRenderQueue::UpdateFrame(IRender& render) {
	render.YieldQueue(queue);
	++invokeCounter;
}

IRender::Queue* ZFencedRenderQueue::GetQueue() const {
	return queue;
}

bool ZFencedRenderQueue::WaitUpdate() const {
	return invokeCounter.load(std::memory_order_relaxed) == 0;
}