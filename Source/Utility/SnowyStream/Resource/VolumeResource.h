// VolumeResource.h
// By PaintDream (paintdream@paintdream.com)
// 2018-3-10
//

#pragma once
#include "GraphicResourceBase.h"
#include "../../../General/Interface/IAsset.h"

namespace PaintsNow {
	class VolumeResource : public TReflected<VolumeResource, GraphicResourceBase> {
	public:
		VolumeResource(ResourceManager& manager, const String& uniqueID);
		virtual bool operator << (IStreamBase& stream) override;
		virtual bool operator >> (IStreamBase& stream) const override;
		virtual void Upload(IRender& render, void* deviceContext) override;
		virtual void Download(IRender& render, void* deviceContext) override;
		virtual void Attach(IRender& render, void* deviceContext) override;
		virtual void Detach(IRender& render, void* deviceContext) override;
	};
}

