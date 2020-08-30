#include "RenderPortCommandQueue.h"

using namespace PaintsNow;

RenderPortCommandQueue::RenderPortCommandQueue() : renderQueue(nullptr) {}

RenderPortCommandQueue::~RenderPortCommandQueue() {
	assert(renderQueue == nullptr);
}

TObject<IReflect>& RenderPortCommandQueue::operator () (IReflect& reflect) {
	BaseClass::operator () (reflect);

	return *this;
}

bool RenderPortCommandQueue::BeginFrame(IRender& render) {
	return true;
}

void RenderPortCommandQueue::EndFrame(IRender& render) {
	render.FlushQueue(renderQueue);
}

void RenderPortCommandQueue::CheckinState(IRender& render, IRender::Resource* stateResource) {
	render.ExecuteResource(renderQueue, stateResource);
}

void RenderPortCommandQueue::DrawElement(IRender& render, IRender::Resource* drawCallResource) {
	render.ExecuteResource(renderQueue, drawCallResource);
}

void RenderPortCommandQueue::Initialize(IRender& render, IRender::Queue* mainQueue) {
	renderQueue = render.CreateQueue(render.GetQueueDevice(mainQueue), IRender::QUEUE_REPEATABLE);
}

void RenderPortCommandQueue::Uninitialize(IRender& render, IRender::Queue* mainQueue) {
	bool deleteQueue = false;
	while (!mergedQueues.Empty()) {
		IRender::Queue* q = mergedQueues.Top();
		if (q != nullptr) {
			if (deleteQueue) {
				render.DeleteQueue(q);
				deleteQueue = false;
			}
		} else {
			deleteQueue = true;
		}

		mergedQueues.Pop();
	}

	render.DeleteQueue(renderQueue);
	renderQueue = nullptr;
}

void RenderPortCommandQueue::Commit(std::vector<IRender::Queue*>& fencedQueues, std::vector<IRender::Queue*>& instanceQueues, std::vector<IRender::Queue*>& deletedQueues) {
	bool deleteQueue = false;
	while (!mergedQueues.Empty()) {
		IRender::Queue* q = mergedQueues.Top();
		if (q != nullptr) {
			instanceQueues.emplace_back(q);

			if (deleteQueue) {
				deletedQueues.emplace_back(q);
				deleteQueue = false;
			}
		} else {
			deleteQueue = true;
		}

		mergedQueues.Pop();
	}

	fencedQueues.emplace_back(renderQueue);
}

bool RenderPortCommandQueue::UpdateDataStream(RenderPort& source) {
	return true;
}

void RenderPortCommandQueue::MergeQueue(IRender& render, IRender::Queue* queue) {
	mergedQueues.Push(queue);
}

void RenderPortCommandQueue::DeleteMergedQueue(IRender& render, IRender::Queue* queue) {
	static IRender::Queue* mark = nullptr;
	mergedQueues.Push(mark); // mark for execute and delete
	mergedQueues.Push(queue);
}