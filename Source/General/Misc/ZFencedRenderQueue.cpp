#include "ZFencedRenderQueue.h"

using namespace PaintsNow;

ZFencedRenderQueue::ZFencedRenderQueue() : queue(nullptr) {
	yieldCount.store(0, std::memory_order_relaxed);
}

ZFencedRenderQueue::~ZFencedRenderQueue() {
	assert(queue == nullptr);
}

void ZFencedRenderQueue::Initialize(IRender& render, IRender::Device* device) {
	assert(queue == nullptr);
	queue = render.CreateQueue(device);
	render.YieldQueue(queue);
}

void ZFencedRenderQueue::Uninitialize(IRender& render) {
	assert(queue != nullptr);
	render.DeleteQueue(queue);
	queue = nullptr;
}

void ZFencedRenderQueue::InvokeRenderQueuesParallel(IRender& render, std::vector<ZFencedRenderQueue*>& queues) {
	if (queues.empty()) return;

	// cleanup stage
	std::vector<IRender::Queue*> renderQueues;
	while (true) {
		for (size_t i = 0; i < queues.size(); i++) {
			ZFencedRenderQueue* q = queues[i];
			if (q->yieldCount.load(std::memory_order_acquire) != 0) {
				renderQueues.emplace_back(q->queue);
				q->yieldCount.fetch_sub(1, std::memory_order_relaxed);
			}
		}

		if (renderQueues.empty()) break;

		render.PresentQueues(&renderQueues[0], (uint32_t)renderQueues.size(), IRender::PRESENT_CONSUME_YIELD);
		renderQueues.clear();
	}

	for (size_t i = 0; i < queues.size(); i++) {
		ZFencedRenderQueue* q = queues[i];
		renderQueues.emplace_back(q->GetQueue());
	}

	render.PresentQueues(&renderQueues[0], (uint32_t)renderQueues.size(), IRender::PRESENT_REPEAT_TO_YIELD);
}

void ZFencedRenderQueue::InvokeRender(IRender& render) {
	assert(queue != nullptr);
	while (yieldCount.load(std::memory_order_acquire) != 0) {
		render.PresentQueues(&queue, 1, IRender::PRESENT_CONSUME_YIELD);
		yieldCount.fetch_sub(1, std::memory_order_relaxed);
	}

	render.PresentQueues(&queue, 1, IRender::PRESENT_EXECUTE_TO_YIELD);
}

void ZFencedRenderQueue::UpdateFrame(IRender& render) {
	render.YieldQueue(queue);
	++yieldCount;
}

IRender::Queue* ZFencedRenderQueue::GetQueue() const {
	return queue;
}

bool ZFencedRenderQueue::WaitUpdate() const {
	return yieldCount.load(std::memory_order_relaxed) == 0;
}