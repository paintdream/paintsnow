// TextShadingFS.h
// By PaintDream (paintdream@paintdream.com)
// 2018-4-13
//

#ifndef __TEXTSHADING_FS_H
#define __TEXTSHADING_FS_H

#include "../../../../General/Interface/IShader.h"

namespace PaintsNow {
	namespace NsSnowyStream {
		class TextShadingFS : public TReflected<TextShadingFS, IShader> {
		public:
			TextShadingFS();
			virtual TObject<IReflect>& operator () (IReflect& reflect) override;
			virtual String GetShaderText() override;

		public:
			IShader::BindTexture mainTexture;
			Float2 rasterCoord;

			// targets
			Float4 target;
		};
	}
}


#endif // __TEXTSHADING_FS_H