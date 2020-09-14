// TextTransformVS.h
// By PaintDream (paintdream@paintdream.com)
// 2018-4-13
//

#ifndef __TEXTTRANSFORM_VS_H
#define __TEXTTRANSFORM_VS_H

#include "../../../../General/Interface/IShader.h"

namespace PaintsNow {
	class TextTransformVS : public TReflected<TextTransformVS, IShader> {
	public:
		TextTransformVS();

		TObject<IReflect>& operator () (IReflect& reflect) override;
		String GetShaderText() override;

		IShader::BindBuffer positionBuffer;
		IShader::BindBuffer instanceBuffer;
		IShader::BindBuffer texCoordRectBuffer;

		// Instanced
		MatrixFloat4x4 worldMatrix;
		Float4 texCoordRect;

		// Input
		Float4 unitTexCoord;

		// Output
		Float2 rasterCoord;
		Float4 rasterPosition;
	};
}

#endif // __TEXTTRANSFORM_VS_H
