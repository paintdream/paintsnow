#include "BatchComponent.h"
#include "../Renderable/RenderableComponent.h"
#include "../../../SnowyStream/SnowyStream.h"

using namespace PaintsNow;
using namespace PaintsNow::NsMythForest;
using namespace PaintsNow::NsSnowyStream;

BatchComponent::BatchComponent() : referenceCount(0), buffer(nullptr), queue(nullptr) {}

BatchComponent::~BatchComponent() {
	assert(referenceCount == 0);
}

void BatchComponent::Initialize(Engine& engine, Entity* entity) {
	BaseClass::Initialize(engine, entity);
}

void BatchComponent::Uninitialize(Engine& engine, Entity* entity) {
	BaseClass::Uninitialize(engine, entity);
}

void BatchComponent::InstanceInitialize(Engine& engine) {
	if (referenceCount == 0) {
		IRender& render = engine.interfaces.render;
		queue = render.CreateQueue(engine.snowyStream.GetRenderDevice());
		buffer = render.CreateResource(queue, IRender::Resource::RESOURCE_BUFFER);
	}

	++referenceCount;
}

void BatchComponent::InstanceUninitialize(Engine& engine) {
	assert(referenceCount != 0);
	if (--referenceCount == 0) {
		IRender& render = engine.interfaces.render;
		render.DeleteResource(queue, buffer);
		render.DeleteQueue(queue);
		queue = nullptr;

		Flag() &= ~Tiny::TINY_MODIFIED;
	}
}

void BatchComponent::DispatchEvent(Event& event, Entity* entity) {
	if (event.eventID == Event::EVENT_FRAME) {
		if (Flag() & Tiny::TINY_UPDATING) {
			Engine& engine = event.engine;
			engine.interfaces.render.PresentQueues(&queue, 1, IRender::CONSUME);
			Flag() &= ~(Tiny::TINY_MODIFIED | Tiny::TINY_UPDATING);
		}
	}
}

void BatchComponent::Flush(IRender& render) {
	if (!(Flag() & Tiny::TINY_UPDATING) && (Flag() & Tiny::TINY_MODIFIED)) {
		IRender::Resource::BufferDescription desc;
		desc.component = 4; // packed by float4
		desc.format = IRender::Resource::BufferDescription::FLOAT;
		desc.dynamic = 0;
		desc.usage = IRender::Resource::BufferDescription::UNIFORM;
		desc.data = currentData;
		render.UploadResource(queue, buffer, &desc);

		Flag() |= Tiny::TINY_UPDATING;
	}
}

IRender::Resource::DrawCallDescription::BufferRange BatchComponent::Allocate(IRender& render, const Bytes& data) {
	assert(!(Flag() & Tiny::TINY_UPDATING));
	uint32_t appendSize = data.GetSize();
	uint32_t curSize = currentData.GetSize();
	currentData.Append(data);

	IRender::Resource::DrawCallDescription::BufferRange bufferRange;
	bufferRange.buffer = buffer;
	bufferRange.offset = curSize;
	bufferRange.length = appendSize;
	Flag() |= Tiny::TINY_MODIFIED;

	return bufferRange;
}
