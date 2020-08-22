// ZFontFreetype.h
// By PaintDream (paintdream@paintdream.com)
// 2014-12-16
//

#pragma once
#include "../../../Interface/IFontBase.h"
#include <iconv.h>

namespace PaintsNow {
	class ZFontFreetype final : public IFontBase {
	public:
		virtual Font* Load(IStreamBase& stream, size_t length);
		virtual void Close(Font* font);
		virtual CHARINFO RenderTexture(Font* font, String& data, FONTCHAR character, size_t bitmapSiz, float hinting) const;
	};
}
