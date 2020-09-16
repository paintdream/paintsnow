#include "RenderPortRenderTarget.h"
#include "../RenderStage.h"

using namespace PaintsNow;

// RenderPortRenderTargetLoad

RenderPortRenderTargetLoad::RenderPortRenderTargetLoad(IRender::Resource::RenderTargetDescription::Storage& storage, bool write) : bindingStorage(storage) {
	if (write) {
		Flag().fetch_or(Tiny::TINY_UNIQUE, std::memory_order_acquire);
	}
}

TObject<IReflect>& RenderPortRenderTargetLoad::operator () (IReflect& reflect) {
	BaseClass::operator () (reflect);

	if (reflect.IsReflectProperty()) {}

	return *this;
}

void RenderPortRenderTargetLoad::Initialize(IRender& render, IRender::Queue* mainQueue) {}
void RenderPortRenderTargetLoad::Uninitialize(IRender& render, IRender::Queue* mainQueue) {}

void RenderPortRenderTargetLoad::Tick(Engine& engine, IRender::Queue* queue) {
	if (GetLinks().empty()) return;

	RenderPort* port = static_cast<RenderPort*>(GetLinks().back().port);
	RenderPortRenderTargetStore* target = port->QueryInterface(UniqueType<RenderPortRenderTargetStore>());
	if (target != nullptr) {
		RenderStage* renderStage = static_cast<RenderStage*>(port->GetNode());
		RenderStage* hostRenderStage = static_cast<RenderStage*>(GetNode());
		const IRender::Resource::RenderTargetDescription& desc = renderStage->GetRenderTargetDescription();
		const IRender::Resource::RenderTargetDescription& hostDesc = hostRenderStage->GetRenderTargetDescription();

		if (&bindingStorage == &hostDesc.depthStorage) {
			bindingStorage.resource = target->bindingStorage.resource;
			bindingStorage.mipLevel = target->bindingStorage.mipLevel;
		} else {
			size_t index = &target->bindingStorage - &desc.colorStorages[0];
			bindingStorage.resource = desc.colorStorages[index].resource;
			bindingStorage.mipLevel = desc.colorStorages[index].mipLevel;
		}

		// Link to correspond RenderTargetStore
		const std::vector<RenderStage::PortInfo>& ports = hostRenderStage->GetPorts();
		for (size_t i = 0; i < ports.size(); i++) {
			const RenderStage::PortInfo& port = ports[i];
			RenderPortRenderTargetStore* t = port.port->QueryInterface(UniqueType<RenderPortRenderTargetStore>());
			if (t != nullptr && &t->bindingStorage == &bindingStorage) {
				t->attachedTexture = target->attachedTexture;
				break;
			}
		}

		Flag().fetch_or(TINY_MODIFIED, std::memory_order_relaxed);
	}
}

// RenderPortRenderTargetStore

RenderPortRenderTargetStore::RenderPortRenderTargetStore(IRender::Resource::RenderTargetDescription::Storage& storage) : bindingStorage(storage) {}

TObject<IReflect>& RenderPortRenderTargetStore::operator () (IReflect& reflect) {
	BaseClass::operator () (reflect);

	if (reflect.IsReflectProperty()) {
		ReflectProperty(renderTargetDescription);
	}

	return *this;
}

void RenderPortRenderTargetStore::Initialize(IRender& render, IRender::Queue* mainQueue) {}

void RenderPortRenderTargetStore::Uninitialize(IRender& render, IRender::Queue* mainQueue) {
}

void RenderPortRenderTargetStore::Tick(Engine& engine, IRender::Queue* queue) {
	assert(attachedTexture);
	if (attachedTexture) {
		bindingStorage.resource = attachedTexture->GetRenderResource();
		GetNode()->Flag().fetch_or(TINY_MODIFIED, std::memory_order_relaxed);
	}
}