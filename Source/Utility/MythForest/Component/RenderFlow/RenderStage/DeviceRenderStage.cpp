#include "DeviceRenderStage.h"
#include "../RenderPort/RenderPortRenderTarget.h"

using namespace PaintsNow;

DeviceRenderStage::DeviceRenderStage(const String& config) : BaseClass(0) {}

TObject<IReflect>& DeviceRenderStage::operator () (IReflect& reflect) {
	BaseClass::operator () (reflect);

	if (reflect.IsReflectProperty()) {
		ReflectProperty(InputColor);
	}

	return *this;
}

void DeviceRenderStage::SetMainResolution(Engine& engine, IRender::Queue* queue, uint32_t width, uint32_t height) {}
void DeviceRenderStage::UpdatePass(Engine& engine, IRender::Queue* queue) {}
void DeviceRenderStage::Commit(Engine& engine, std::vector<FencedRenderQueue*>& queues, std::vector<IRender::Queue*>& instantQueues, IRender::Queue* queue) {}

void DeviceRenderStage::Tick(Engine& engine, IRender::Queue* queue) {}

void DeviceRenderStage::PrepareResources(Engine& engine, IRender::Queue* queue) {
	assert(InputColor.GetLinks().size() == 1);
	RenderPort* renderPort = static_cast<RenderPort*>(InputColor.GetLinks()[0].port);
	assert(renderPort != nullptr);
	assert(renderPort->QueryInterface(UniqueType<RenderPortRenderTarget>()) != nullptr);
	RenderPortRenderTarget* input = renderPort->QueryInterface(UniqueType<RenderPortRenderTarget>());

	// Clear source node renderTarget
	input->renderTargetTextureResource = nullptr;
	(static_cast<RenderStage*>(input->GetNode()))->renderTargetDescription.colorStorages.clear();

	// BaseClass::PrepareResources(engine, queue);
}
