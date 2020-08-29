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
	render.DeleteQueue(renderQueue);
	renderQueue = nullptr;
}

void RenderPortCommandQueue::Commit(std::vector<IRender::Queue*>& fencedQueues, std::vector<IRender::Queue*>& instanceQueues) {
	while (!mergedQueues.Empty()) {
		instanceQueues.emplace_back(mergedQueues.Top());
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