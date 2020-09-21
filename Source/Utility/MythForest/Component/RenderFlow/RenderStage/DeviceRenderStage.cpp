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

void DeviceRenderStage::SetMainResolution(Engine& engine, IRender::Queue* queue, UShort2 res) {}
void DeviceRenderStage::UpdatePass(Engine& engine, IRender::Queue* queue) {}
void DeviceRenderStage::Commit(Engine& engine, std::vector<IRender::Queue*>& queues, std::vector<IRender::Queue*>& instantQueues, std::vector<IRender::Queue*>& deletedQueues, IRender::Queue* instantQueue) {}

void DeviceRenderStage::Tick(Engine& engine, IRender::Queue* queue) {
	BaseClass::Tick(engine, queue);
}

void DeviceRenderStage::PrepareResources(Engine& engine, IRender::Queue* queue) {
	assert(InputColor.GetLinks().size() == 1);
	RenderPort* renderPort = static_cast<RenderPort*>(InputColor.GetLinks()[0].port);
	assert(renderPort != nullptr);
	assert(renderPort->QueryInterface(UniqueType<RenderPortRenderTargetStore>()) != nullptr);
	RenderPortRenderTargetStore* input = renderPort->QueryInterface(UniqueType<RenderPortRenderTargetStore>());

	// Hack
	(static_cast<RenderStage*>(input->GetNode()))->Flag().fetch_or(RenderStage::RENDERSTAGE_OUTPUT_TO_BACK_BUFFER, std::memory_order_relaxed);

	for (RenderPortRenderTargetLoad* loader = input->QueryLoad(); loader != nullptr; loader = input->QueryLoad()) {
		if (loader->GetLinks().empty()) break;

		input = loader->GetLinks().back().port->QueryInterface(UniqueType<RenderPortRenderTargetStore>());

		if (input == nullptr) break;

		(static_cast<RenderStage*>(input->GetNode()))->Flag().fetch_or(RenderStage::RENDERSTAGE_OUTPUT_TO_BACK_BUFFER, std::memory_order_relaxed);
	}

	// BaseClass::PrepareResources(engine, queue);
}
