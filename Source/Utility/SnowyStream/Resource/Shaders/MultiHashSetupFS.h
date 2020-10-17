// MultiHashSetupFS.h
// By PaintDream (paintdream@paintdream.com)
// 2018-4-13
//

#pragma once
#include "../../../../General/Interface/IShader.h"

namespace PaintsNow {
	class MultiHashSetupFS : public TReflected<MultiHashSetupFS, IShader> {
	public:
		MultiHashSetupFS();
		TObject<IReflect>& operator () (IReflect& reflect) override;
		String GetShaderText() override;

		BindTexture noiseTexture;

		Float4 rasterCoord;
		Float4 tintColor;
	};
}


