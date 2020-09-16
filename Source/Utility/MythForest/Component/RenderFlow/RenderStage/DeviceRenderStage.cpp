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
void DeviceRenderStage::Commit(Engine& engine, std::vector<IRender::Queue*>& queues, std::vector<IRender::Queue*>& instantQueues, std::vector<IRender::Queue*>& deletedQueues, IRender::Queue* instantQueue) {}

void DeviceRenderStage::Tick(Engine& engine, IRender::Queue* queue) {}

void DeviceRenderStage::PrepareResources(Engine& engine, IRender::Queue* queue) {
	assert(InputColor.GetLinks().size() == 1);
	RenderPort* renderPort = static_cast<RenderPort*>(InputColor.GetLinks()[0].port);
	assert(renderPort != nullptr);
	assert(renderPort->QueryInterface(UniqueType<RenderPortRenderTargetStore>()) != nullptr);
	RenderPortRenderTargetStore* input = renderPort->QueryInterface(UniqueType<RenderPortRenderTargetStore>());

	assert(false);
	// TODO
	// Clear source node renderTarget
	/*
	input->renderTargetDescription = nullptr;
	(static_cast<RenderStage*>(input->GetNode()))->renderTargetDescription.colorStorages.clear();
*/
	// BaseClass::PrepareResources(engine, queue);
}
