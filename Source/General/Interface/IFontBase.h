// IFontBase.h -- Font interfaces
// PaintDream (paintdream@paintdream.com)
// 2014-12-1
//

#ifndef __IFONTBASE_H__
#define __IFONTBASE_H__


#include "../../Core/PaintsNow.h"
#include "../../Core/Template/TMatrix.h"
#include "../../Core/Interface/IStreamBase.h"
#include "IRender.h"
#include "../../Core/Interface/IDevice.h"

#if defined(_MSC_VER) && _MSC_VER <= 1200
typedef unsigned __int64 uint64_t;
#else
#include <cstdint>
#endif

namespace PaintsNow {
	class IStreamBase;
	class IFontBase : public IDevice {
	public:
		typedef uint32_t FONTCHAR;
		struct CHARINFO {
			int height;
			int width;
			Int2 adv;
			Int2 delta;
			Int2 bearing;
		};

		class Font {};

		virtual ~IFontBase();
		virtual Font* Load(IStreamBase& stream, size_t length) = 0;
		virtual void Close(Font* font) = 0;
		virtual CHARINFO RenderTexture(Font* font, String& data, FONTCHAR character, size_t bitmapSize, float hinting) const = 0;
	};
}


#endif // __IFONTBASE_H__