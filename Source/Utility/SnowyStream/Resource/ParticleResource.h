// ParticleResource.h
// By PaintDream (paintdream@paintdream.com)
// 2018-3-10
//

#pragma once
#include "GraphicResourceBase.h"
#include "../../../General/Interface/IAsset.h"

namespace PaintsNow {
	class ParticleResource : public TReflected<ParticleResource, GraphicResourceBase> {
	public:
		ParticleResource(ResourceManager& manager, const String& uniqueID);
		bool operator << (IStreamBase& stream) override;
		bool operator >> (IStreamBase& stream) const override;
		void Upload(IRender& render, void* deviceContext) override;
		void Download(IRender& render, void* deviceContext) override;
		void Attach(IRender& render, void* deviceContext) override;
		void Detach(IRender& render, void* deviceContext) override;

	};
}

