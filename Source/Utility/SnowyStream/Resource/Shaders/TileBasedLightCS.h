// DeferredCompact.h
// By PaintDream (paintdream@paintdream.com)
// 2018-4-13
//

#pragma once
#include "../../../../General/Interface/IShader.h"

namespace PaintsNow {
	class TileBasedLightCS : public TReflected<TileBasedLightCS, IShader> {
	public:
		TileBasedLightCS();
		TObject<IReflect>& operator () (IReflect& reflect) override;
		String GetShaderText() override;
	};
}


