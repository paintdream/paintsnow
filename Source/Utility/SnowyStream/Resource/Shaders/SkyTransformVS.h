// SkyTransformVS.h
// PaintDream (paintdream@paintdream.com)
// 2018-4-13
//

#pragma once
#include "../../../../General/Interface/IShader.h"

namespace PaintsNow {
	class SkyTransformVS : public TReflected<SkyTransformVS, IShader> {
	public:
		SkyTransformVS();

		TObject<IReflect>& operator () (IReflect& reflect) override;
		String GetShaderText() override;
	};
}


