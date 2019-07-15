#include "RenderPortLightSource.h"
#include "../../Light/LightComponent.h"

using namespace PaintsNow;
using namespace PaintsNow::NsMythForest;

RenderPortLightSource::RenderPortLightSource() : stencilMask(0) {}

TObject<IReflect>& RenderPortLightSource::operator () (IReflect& reflect) {
	BaseClass::operator () (reflect);

	return *this;
}

void RenderPortLightSource::Initialize(IRender& render, IRender::Queue* mainQueue) {
}

void RenderPortLightSource::Uninitialize(IRender& render, IRender::Queue* mainQueue) {
}

bool RenderPortLightSource::UpdateDataStream(RenderPort& source) {
	return true;
}

bool RenderPortLightSource::BeginFrame(IRender& render) {
	lightElements.clear();
	return true;
}

void RenderPortLightSource::EndFrame(IRender& render) {
	node->Flag() |= TINY_MODIFIED;
	for (PortMap::const_iterator it = targetPorts.begin(); it != targetPorts.end(); ++it) {
		RenderPort* renderPort = static_cast<RenderPort*>(it->first);
		renderPort->GetNode()->Flag() |= TINY_MODIFIED;
	}
}