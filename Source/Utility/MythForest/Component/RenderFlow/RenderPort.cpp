#include "RenderPort.h"

using namespace PaintsNow;

TObject<IReflect>& RenderPort::operator () (IReflect& reflect) {
	typedef GraphPort<SharedTiny> Base;
	BaseClass::operator () (reflect);

	return *this;
}

bool RenderPort::BeginFrame(IRender& render) {
	return true;
}

void RenderPort::EndFrame(IRender& render) {}

void RenderPort::Tick(Engine& engine, IRender::Queue* queue) {
	eventTickHooks(engine, *this, queue);
}

void RenderPort::Initialize(IRender& render, IRender::Queue* mainQueue) {}
void RenderPort::Uninitialize(IRender& render, IRender::Queue* mainQueue) {}

void RenderPort::UpdateRenderStage() {
	Tiny* renderStage = GetNode();
	if (renderStage != nullptr) {
		renderStage->Flag().fetch_or(TINY_MODIFIED, std::memory_order_acquire);
	}
}

void RenderPort::Commit(std::vector<IRender::Queue*>& fencedQueues, std::vector<IRender::Queue*>& instanceQueues) {}
bool RenderPort::UpdateDataStream(RenderPort& source) { return true; }