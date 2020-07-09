#include "BatchComponent.h"
#include "../Renderable/RenderableComponent.h"
#include "../../../MythForest/MythForest.h"

using namespace PaintsNow;
using namespace PaintsNow::NsMythForest;
using namespace PaintsNow::NsSnowyStream;

BatchComponent::BatchComponent() : referenceCount(0), buffer(nullptr) {}

BatchComponent::~BatchComponent() {
	assert(referenceCount == 0);
}

void BatchComponent::InstanceInitialize(Engine& engine) {
	if (referenceCount == 0) {
		IRender& render = engine.interfaces.render;
		IRender::Queue* queue = engine.GetWarpResourceQueue();
		buffer = render.CreateResource(queue, IRender::Resource::RESOURCE_BUFFER);
	}

	++referenceCount;
}

void BatchComponent::InstanceUninitialize(Engine& engine) {
	assert(referenceCount != 0);
	if (--referenceCount == 0) {
		IRender& render = engine.interfaces.render;
		IRender::Queue* queue = engine.GetWarpResourceQueue();
		render.DeleteResource(queue, buffer);
		currentData.Clear();
		Flag().fetch_and(~Tiny::TINY_MODIFIED, std::memory_order_release);
	}
}

void BatchComponent::Update(IRender& render, IRender::Queue* queue) {
	if (Flag() & Tiny::TINY_MODIFIED) {
		IRender::Resource::BufferDescription desc;
		desc.component = 4; // packed by float4
		desc.format = IRender::Resource::BufferDescription::FLOAT;
		desc.dynamic = 0;
		desc.usage = IRender::Resource::BufferDescription::UNIFORM;
		desc.data = currentData;
		render.UploadResource(queue, buffer, &desc);

		Flag().fetch_and(~Tiny::TINY_MODIFIED, std::memory_order_release);
	}
}

IRender::Resource::DrawCallDescription::BufferRange BatchComponent::Allocate(const Bytes& data) {
	uint32_t appendSize = data.GetSize();
	uint32_t curSize = currentData.GetSize();
	currentData.Append(data);

	IRender::Resource::DrawCallDescription::BufferRange bufferRange;
	bufferRange.buffer = buffer;
	bufferRange.offset = curSize;
	bufferRange.length = appendSize;
	Flag().fetch_or(Tiny::TINY_MODIFIED, std::memory_order_acquire);

	return bufferRange;
}
