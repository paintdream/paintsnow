#include "RenderPortLightSource.h"
#include "../../Light/LightComponent.h"

using namespace PaintsNow;

RenderPortLightSource::RenderPortLightSource() : stencilMask(0) {}

TObject<IReflect>& RenderPortLightSource::operator () (IReflect& reflect) {
	BaseClass::operator () (reflect);

	return *this;
}

void RenderPortLightSource::Initialize(IRender& render, IRender::Queue* mainQueue) {
}

void RenderPortLightSource::Uninitialize(IRender& render, IRender::Queue* mainQueue) {
}

bool RenderPortLightSource::BeginFrame(IRender& render) {
	lightElements.clear();
	return true;
}

void RenderPortLightSource::EndFrame(IRender& render) {
	node->Flag().fetch_or(TINY_MODIFIED, std::memory_order_acquire);
	for (size_t i = 0; i < links.size(); i++) {
		RenderPort* renderPort = static_cast<RenderPort*>(links[i].port);
		renderPort->GetNode()->Flag().fetch_or(TINY_MODIFIED, std::memory_order_acquire);
	}
}