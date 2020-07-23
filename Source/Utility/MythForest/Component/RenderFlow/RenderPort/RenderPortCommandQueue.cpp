#include "RenderPortCommandQueue.h"

using namespace PaintsNow;
using namespace PaintsNow::NsMythForest;

RenderPortCommandQueue::RenderPortCommandQueue() {
}

TObject<IReflect>& RenderPortCommandQueue::operator () (IReflect& reflect) {
	BaseClass::operator () (reflect);

	return *this;
}

bool RenderPortCommandQueue::BeginFrame(IRender& render) {
	return renderQueue.WaitUpdate();
}

void RenderPortCommandQueue::EndFrame(IRender& render) {
	renderQueue.UpdateFrame(render);
}

void RenderPortCommandQueue::CheckinState(IRender& render, IRender::Resource* stateResource) {
	render.ExecuteResource(renderQueue.GetQueue(), stateResource);
}

void RenderPortCommandQueue::DrawElement(IRender& render, IRender::Resource* drawCallResource) {
	render.ExecuteResource(renderQueue.GetQueue(), drawCallResource);
}

void RenderPortCommandQueue::Initialize(IRender& render, IRender::Queue* mainQueue) {
	renderQueue.Initialize(render, render.GetQueueDevice(mainQueue));
}

void RenderPortCommandQueue::Uninitialize(IRender& render, IRender::Queue* mainQueue) {
	renderQueue.Uninitialize(render);
}

void RenderPortCommandQueue::Commit(std::vector<FencedRenderQueue*>& queues) {
	queues.emplace_back(&renderQueue);
}

bool RenderPortCommandQueue::UpdateDataStream(RenderPort& source) {
	return true;
}

void RenderPortCommandQueue::MergeQueue(IRender& render, IRender::Queue* queue) {
	render.MergeQueue(renderQueue.GetQueue(), queue);
}