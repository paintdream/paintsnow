// TextureArrayResource.h
// PaintDream (paintdream@paintdream.com)
// 2018-3-10
//

#pragma once
#include "RenderResourceBase.h"

namespace PaintsNow {
	class TextureArrayResource : public TReflected<TextureArrayResource, RenderResourceBase> {
	public:
		TextureArrayResource(ResourceManager& manager, const String& uniqueID);

		void Refresh(IRender& device, void* deviceContext) override;
		void Download(IRender& device, void* deviceContext) override;
		void Upload(IRender& device, void* deviceContext) override;
		void Attach(IRender& device, void* deviceContext) override;
		void Detach(IRender& device, void* deviceContext) override;
	};
}

