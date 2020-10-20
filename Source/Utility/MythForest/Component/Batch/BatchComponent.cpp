#include "BatchComponent.h"
#include "../Renderable/RenderableComponent.h"
#include "../../../MythForest/MythForest.h"

using namespace PaintsNow;

BatchComponent::BatchComponent(IRender::Resource::BufferDescription::Usage usage) : referenceCount(0), buffer(nullptr), bufferUsage(usage) {}

BatchComponent::~BatchComponent() {
	assert(referenceCount == 0);
}

void BatchComponent::InstanceInitialize(Engine& engine) {
	if (referenceCount == 0) {
		IRender& render = engine.interfaces.render;
		buffer = render.CreateResource(engine.snowyStream.GetRenderDevice(), IRender::Resource::RESOURCE_BUFFER);
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

IRender::Resource::BufferDescription::Usage BatchComponent::GetBufferUsage() const {
	return bufferUsage;
}

Bytes& BatchComponent::GetCurrentData() {
	return currentData;
}

uint32_t BatchComponent::Update(IRender& render, IRender::Queue* queue) {
	if (Flag() & Tiny::TINY_MODIFIED) {
		IRender::Resource::BufferDescription desc;
		desc.component = 4; // packed by float4
		desc.format = IRender::Resource::BufferDescription::FLOAT;
		desc.dynamic = 0;
		desc.usage = IRender::Resource::BufferDescription::UNIFORM;
		desc.data = currentData;
		render.UploadResource(queue, buffer, &desc);

		Flag().fetch_and(~Tiny::TINY_MODIFIED, std::memory_order_release);
		return 1;
	} else {
		return 0;
	}
}

IRender::Resource::DrawCallDescription::BufferRange BatchComponent::Allocate(const void* data, uint32_t appendSize) {
	size_t curSize = currentData.GetSize();
	currentData.Append(reinterpret_cast<const uint8_t*>(data), appendSize);

	IRender::Resource::DrawCallDescription::BufferRange bufferRange;
	bufferRange.buffer = buffer;
	bufferRange.offset = safe_cast<uint32_t>(curSize);
	bufferRange.length = appendSize;
	Flag().fetch_or(Tiny::TINY_MODIFIED, std::memory_order_release);

	return bufferRange;
}
