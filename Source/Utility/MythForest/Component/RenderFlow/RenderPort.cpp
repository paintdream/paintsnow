#include "RenderPort.h"

using namespace PaintsNow;
using namespace PaintsNow::NsMythForest;

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
		renderStage->Flag() |= TINY_MODIFIED;
	}
}

void RenderPort::Commit(std::vector<ZFencedRenderQueue*>& queues) {}
bool RenderPort::UpdateDataStream(RenderPort& source) { return true; }