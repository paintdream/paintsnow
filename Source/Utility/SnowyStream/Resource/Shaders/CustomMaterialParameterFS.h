// CustomMaterialParameterFS.h
// By PaintDream (paintdream@paintdream.com)
// 2018-4-13
//

#pragma once
#include "CustomMaterialDescription.h"

namespace PaintsNow {
	class CustomMaterialParameterFS : public TReflected<CustomMaterialParameterFS, IShader> {
	public:
		CustomMaterialParameterFS();
		TObject<IReflect>& operator () (IReflect& reflect) override;
		String GetShaderText() override;

		TShared<CustomShaderDescription> description;
	};
}


