// TextShadingFS.h
// By PaintDream (paintdream@paintdream.com)
// 2018-4-13
//

#ifndef __TEXTSHADING_FS_H
#define __TEXTSHADING_FS_H

#include "../../../../General/Interface/IShader.h"

namespace PaintsNow {
	class TextShadingFS : public TReflected<TextShadingFS, IShader> {
	public:
		TextShadingFS();
		TObject<IReflect>& operator () (IReflect& reflect) override;
		String GetShaderText() override;

	public:
		IShader::BindTexture mainTexture;
		Float4 color;
		Float2 texCoord;

		// targets
		Float4 target;
	};
}

#endif // __TEXTSHADING_FS_H