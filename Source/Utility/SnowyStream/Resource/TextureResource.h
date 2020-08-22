// TextureResource.h
// By PaintDream (paintdream@paintdream.com)
// 2018-3-10
//

#pragma once
#include "GraphicResourceBase.h"

namespace PaintsNow {
	class TextureResource : public TReflected<TextureResource, GraphicResourceBase> {
	public:
		TextureResource(ResourceManager& manager, const String& uniqueID);

		virtual size_t ReportDeviceMemoryUsage() const;
		virtual void Upload(IRender& render, void* deviceContext) override;
		virtual void Download(IRender& render, void* deviceContext) override;
		virtual void Attach(IRender& render, void* deviceContext) override;
		virtual void Detach(IRender& render, void* deviceContext) override;
		virtual bool Compress(const String& compressType) override;
		virtual bool LoadExternalResource(Interfaces& interfaces, IStreamBase& streamBase, size_t length);
		virtual void Unmap() override;

		virtual TObject<IReflect>& operator () (IReflect& reflect) override;
		IRender::Resource* GetTexture() const;
		IRender::Resource::TextureDescription description;

	public:
		IRender::Resource* instance;
		size_t deviceMemoryUsage;
	};
}

