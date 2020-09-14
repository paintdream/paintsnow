// BloomFS.h
// By PaintDream (paintdream@paintdream.com)
// 2018-4-13
//

#pragma once
#include "../../../../General/Interface/IShader.h"

namespace PaintsNow {
	class BloomFS : public TReflected<BloomFS, IShader> {
	public:
		BloomFS();
		TObject<IReflect>& operator () (IReflect& reflect) override;
		String GetShaderText() override;

		BindTexture screenTexture;
		BindBuffer uniformBloomBuffer;
		Float2 invScreenSize;

	protected:
		Float2 rasterCoord;
		Float4 outputColor;
	};
}
