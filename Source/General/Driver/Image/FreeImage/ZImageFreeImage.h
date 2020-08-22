// ZImageFreeImage.h
// By PaintDream (paintdream@paintdream.com)
// 2014-12-14
//

#pragma once
#include "../../../../Core/Interface/IStreamBase.h"
#include "../../../Interface/Interfaces.h"
#include "../../../../Core/Interface/IType.h"
typedef struct FIBITMAP* PFIBITMAP;

namespace PaintsNow {
	class ZImageFreeImage final : public IImage {
	public:
		virtual Image* Create(size_t width, size_t height, IRender::Resource::TextureDescription::Layout layout, IRender::Resource::TextureDescription::Format dataType) const override;
		virtual IRender::Resource::TextureDescription::Layout GetLayoutType(Image* image) const override;
		virtual IRender::Resource::TextureDescription::Format GetDataType(Image* image) const override;
		virtual size_t GetBPP(Image* image) const override;
		virtual size_t GetWidth(Image* image) const override;
		virtual size_t GetHeight(Image* image) const override;
		virtual void* GetBuffer(Image* image) const override;
		virtual bool Load(Image* image, IStreamBase& streamBase, size_t length) const override;
		virtual bool Save(Image* image, IStreamBase& streamBase, const String& type) const override;
		virtual void Delete(Image* image) const override;
	};
}

