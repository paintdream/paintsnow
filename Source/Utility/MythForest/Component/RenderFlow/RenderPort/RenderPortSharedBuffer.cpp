#include "RenderPortSharedBuffer.h"
#include "../RenderStage.h"

using namespace PaintsNow;

// RenderPortSharedBufferLoad

RenderPortSharedBufferLoad::RenderPortSharedBufferLoad(bool write) : sharedBufferResource(nullptr) {
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
		bufferSize = bufferStore->bufferSize;
		depthSize = bufferStore->depthSize;
	}
}

// RenderPortSharedBufferStore

RenderPortSharedBufferStore::RenderPortSharedBufferStore() : sharedBufferResource(nullptr) {
}

TObject<IReflect>& RenderPortSharedBufferStore::operator () (IReflect& reflect) {
	BaseClass::operator () (reflect);

	if (reflect.IsReflectProperty()) {
	}

	return *this;
}

void RenderPortSharedBufferStore::Initialize(IRender& render, IRender::Queue* mainQueue) {}

void RenderPortSharedBufferStore::Uninitialize(IRender& render, IRender::Queue* mainQueue) {
	if (sharedBufferResource != nullptr) {
		render.DeleteResource(mainQueue, sharedBufferResource);
		sharedBufferResource = nullptr;
	}
}

void RenderPortSharedBufferStore::Tick(Engine& engine, IRender::Queue* queue) {
	BaseClass::Tick(engine, queue);
}

