#include "RenderPortSharedBuffer.h"
#include "../RenderStage.h"

using namespace PaintsNow;

// RenderPortSharedBufferLoad

RenderPortSharedBufferLoad::RenderPortSharedBufferLoad(bool write) {
	if (write) {
		Flag().fetch_or(Tiny::TINY_UNIQUE, std::memory_order_relaxed);
	}
}

TObject<IReflect>& RenderPortSharedBufferLoad::operator () (IReflect& reflect) {
	BaseClass::operator () (reflect);

	if (reflect.IsReflectProperty()) {}

	return *this;
}

void RenderPortSharedBufferLoad::Initialize(IRender& render, IRender::Queue* mainQueue) {}
void RenderPortSharedBufferLoad::Uninitialize(IRender& render, IRender::Queue* mainQueue) {}

void RenderPortSharedBufferLoad::Tick(Engine& engine, IRender::Queue* queue) {
	BaseClass::Tick(engine, queue);
	if (GetLinks().empty()) return;

	RenderPortSharedBufferStore* bufferStore = GetLinks().back().port->QueryInterface(UniqueType<RenderPortSharedBufferStore>());
	if (bufferStore != nullptr) {
		sharedBufferResource = bufferStore->sharedBufferResource;
	}
}

// RenderPortSharedBufferStore

RenderPortSharedBufferStore::RenderPortSharedBufferStore() {
}

TObject<IReflect>& RenderPortSharedBufferStore::operator () (IReflect& reflect) {
	BaseClass::operator () (reflect);

	if (reflect.IsReflectProperty()) {
	}

	return *this;
}

void RenderPortSharedBufferStore::Initialize(IRender& render, IRender::Queue* mainQueue) {}

void RenderPortSharedBufferStore::Uninitialize(IRender& render, IRender::Queue* mainQueue) {
}

void RenderPortSharedBufferStore::Tick(Engine& engine, IRender::Queue* queue) {
	BaseClass::Tick(engine, queue);
}

