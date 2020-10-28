// CustomMaterialTransformVS.h
// By PaintDream (paintdream@paintdream.com)
// 2018-4-13
//

#pragma once
#include "CustomMaterialDescription.h"
#include "StandardTransformVS.h"

namespace PaintsNow {
	class CustomMaterialTransformVS : public TReflected<CustomMaterialTransformVS, IShader> {
	public:
		CustomMaterialTransformVS();
		TObject<IReflect>& operator () (IReflect& reflect) override;
		String GetShaderText() override;

		TShared<CustomShaderDescription> description;
		Bytes uniformData;
		Bytes optionData;
	};
}


