// ScreenTransformVS.h
// By PaintDream (paintdream@paintdream.com)
// 2018-4-13
//

#ifndef __SCREENTRANSFORM_VS_H
#define __SCREENTRANSFORM_VS_H

#include "../../../../General/Interface/IShader.h"

namespace PaintsNow {
	class ScreenTransformVS : public TReflected<ScreenTransformVS, IShader> {
	public:
		ScreenTransformVS();
		TObject<IReflect>& operator () (IReflect& reflect) override;
		String GetShaderText() override;

		IShader::BindBuffer vertexBuffer;

	protected:
		Float3 unitPositionTexCoord;
		Float2 rasterCoord;

		// Output vars
		Float4 position;
	};
}

#endif // __SCREENTRANSFORM_VS_H