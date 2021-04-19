// TextureResource.h
// PaintDream (paintdream@paintdream.com)
// 2018-3-10
//

#pragma once
#include "RenderResourceBase.h"

namespace PaintsNow {
	class TextureResource : public TReflected<TextureResource, RenderResourceBase> {
	public:
		TextureResource(ResourceManager& manager, const String& uniqueID);

		size_t ReportDeviceMemoryUsage() const override;
		void Upload(IRender& render, void* deviceContext) override;
		void Download(IRender& render, void* deviceContext) override;
		void Attach(IRender& render, void* deviceContext) override;
		void Detach(IRender& render, void* deviceContext) override;
		bool Compress(const String& compressType) override;
		bool LoadExternalResource(Interfaces& interfaces, IStreamBase& streamBase, size_t length) override;
		void Unmap() override;

		TObject<IReflect>& operator () (IReflect& reflect) override;
		IRender::Resource* GetRenderResource() const;
		IRender::Resource::TextureDescription description;

	private:
		IRender::Resource* instance;
		size_t deviceMemoryUsage;
	};
}

