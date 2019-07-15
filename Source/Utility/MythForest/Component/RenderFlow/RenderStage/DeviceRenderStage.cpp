#include "DeviceRenderStage.h"
#include "../RenderPort/RenderPortRenderTarget.h"

using namespace PaintsNow;
using namespace PaintsNow::NsMythForest;

DeviceRenderStage::DeviceRenderStage(const String& config) {}

TObject<IReflect>& DeviceRenderStage::operator () (IReflect& reflect) {
	BaseClass::operator () (reflect);

	if (reflect.IsReflectProperty()) {
		ReflectProperty(InputColor);
	}

	return *this;
}

void DeviceRenderStage::PrepareResources(Engine& engine) {
	renderTargetDescription.isBackBuffer = true;
	const RenderPort::PortMap& portMap = InputColor.GetTargetPortMap();
	assert(portMap.size() == 1);
	RenderPort* renderPort = static_cast<RenderPort*>(portMap.begin()->first);
	assert(renderPort != nullptr);
	assert(renderPort->QueryInterface(UniqueType<RenderPortRenderTarget>()) != nullptr);
	RenderPortRenderTarget* input = renderPort->QueryInterface(UniqueType<RenderPortRenderTarget>());

	// Clear source node renderTarget
	input->renderTargetTextureResource = nullptr;
	(static_cast<RenderStage*>(input->GetNode()))->renderTargetDescription.isBackBuffer = true;

	BaseClass::PrepareResources(engine);
}

void DeviceRenderStage::UpdatePass(Engine& engine) {
	BaseClass::UpdatePass(engine);
}
