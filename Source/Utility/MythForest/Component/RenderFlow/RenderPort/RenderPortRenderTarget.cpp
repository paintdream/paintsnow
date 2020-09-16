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

bool RenderPortRenderTargetLoad::UpdateDataStream(RenderPort& source) {
	RenderStage* renderStage = static_cast<RenderStage*>(source.GetNode());
	RenderPortRenderTargetStore* target = source.QueryInterface(UniqueType<RenderPortRenderTargetStore>());
	if (target != nullptr) {
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
	}

	return true;
}

// RenderPortRenderTargetStore

RenderPortRenderTargetStore::RenderPortRenderTargetStore(IRender::Resource::RenderTargetDescription::Storage& storage) : bindingStorage(storage) {}

TObject<IReflect>& RenderPortRenderTargetStore::operator () (IReflect& reflect) {
	BaseClass::operator () (reflect);

	if (reflect.IsReflectProperty()) {
		ReflectProperty(renderTargetTextureResource);
	}

	return *this;
}

void RenderPortRenderTargetStore::Initialize(IRender& render, IRender::Queue* mainQueue) {
}

void RenderPortRenderTargetStore::Uninitialize(IRender& render, IRender::Queue* mainQueue) {
}

bool RenderPortRenderTargetStore::UpdateDataStream(RenderPort& source) {
	// Sync texture
	RenderPortRenderTargetStore* target = source.QueryInterface(UniqueType<RenderPortRenderTargetStore>());
	if (target != nullptr) {
		renderTargetTextureResource = target->renderTargetTextureResource;
		return true;
	} else {
		return false;
	}
}