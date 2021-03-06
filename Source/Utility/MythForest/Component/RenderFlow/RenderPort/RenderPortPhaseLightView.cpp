#include "RenderPortPhaseLightView.h"
#include "../../Light/LightComponent.h"

using namespace PaintsNow;

RenderPortPhaseLightView::RenderPortPhaseLightView() {}

TObject<IReflect>& RenderPortPhaseLightView::operator () (IReflect& reflect) {
	BaseClass::operator () (reflect);

	return *this;
}

void RenderPortPhaseLightView::Initialize(IRender& render, IRender::Queue* mainQueue) {
}

void RenderPortPhaseLightView::Uninitialize(IRender& render, IRender::Queue* mainQueue) {
}

bool RenderPortPhaseLightView::BeginFrame(IRender& render) {
	return true;
}

void RenderPortPhaseLightView::EndFrame(IRender& render) {
	node->Flag().fetch_or(TINY_MODIFIED, std::memory_order_relaxed);
	for (size_t i = 0; i < links.size(); i++) {
		RenderPort* renderPort = static_cast<RenderPort*>(links[i].port);
		renderPort->GetNode()->Flag().fetch_or(TINY_MODIFIED, std::memory_order_relaxed);
	}
}