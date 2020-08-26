#include "RenderPortLoadTarget.h"
#include "RenderPortRenderTarget.h"
#include "../RenderStage.h"

using namespace PaintsNow;

// RenderPortLoadTarget

RenderPortLoadTarget::RenderPortLoadTarget(IRender::Resource::RenderTargetDescription::Storage& storage, bool write) : bindingStorage(storage) {
	if (write) {
		Flag().fetch_or(Tiny::TINY_UNIQUE, std::memory_order_acquire);
	}
}

TObject<IReflect>& RenderPortLoadTarget::operator () (IReflect& reflect) {
	BaseClass::operator () (reflect);

	if (reflect.IsReflectProperty()) {}

	return *this;
}

void RenderPortLoadTarget::Initialize(IRender& render, IRender::Queue* mainQueue) {
}

void RenderPortLoadTarget::Uninitialize(IRender& render, IRender::Queue* mainQueue) {
}

bool RenderPortLoadTarget::UpdateDataStream(RenderPort& source) {
	RenderStage* renderStage = static_cast<RenderStage*>(source.GetNode());
	RenderPortRenderTarget* target = source.QueryInterface(UniqueType<RenderPortRenderTarget>());
	if (target != nullptr) {
		RenderStage* hostRenderStage = static_cast<RenderStage*>(GetNode());
		const IRender::Resource::RenderTargetDescription& desc = renderStage->GetRenderTargetDescription();
		const IRender::Resource::RenderTargetDescription& hostDesc = hostRenderStage->GetRenderTargetDescription();

		if (&bindingStorage == &hostDesc.depthStorage) {
			bindingStorage.resource = target->bindingStorage.resource;
			bindingStorage.mipLevel = target->bindingStorage.mipLevel;
		} else {
			size_t index = &target->bindingStorage - &desc.colorBufferStorages[0];
			bindingStorage.resource = desc.colorBufferStorages[index].resource;
			bindingStorage.mipLevel = desc.colorBufferStorages[index].mipLevel;
		}
	}

	return true;
}