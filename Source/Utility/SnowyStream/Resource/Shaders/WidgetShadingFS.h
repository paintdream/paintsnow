// WidgetShadingFS.h
// By PaintDream (paintdream@paintdream.com)
// 2018-4-13
//

#pragma once
#include "../../../../General/Interface/IShader.h"

namespace PaintsNow {
	class WidgetShadingFS : public TReflected<WidgetShadingFS, IShader> {
	public:
		WidgetShadingFS();
		TObject<IReflect>& operator () (IReflect& reflect) override;
		String GetShaderText() override;

	public:
		IShader::BindTexture mainTexture;

	protected:
		// varyings
		Float4 texCoord;
		Float4 texCoordRect;
		Float4 texCoordMark;
		Float4 texCoordScale;
		Float4 tintColor;

		// targets
		Float4 target;
	};
}

