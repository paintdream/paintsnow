// WidgetTransformVS.h
// PaintDream (paintdream@paintdream.com)
// 2018-4-13
//

#pragma once
#include "../../../../General/Interface/IShader.h"

namespace PaintsNow {
	class WidgetTransformVS : public TReflected<WidgetTransformVS, IShader> {
	public:
		WidgetTransformVS();
		TObject<IReflect>& operator () (IReflect& reflect) override;

		String GetShaderText() override;

	public:
		IShader::BindBuffer instanceBuffer;
		IShader::BindBuffer vertexBuffer;

		// Input attribs
		Float3 vertexPosition;

		// instanced
		MatrixFloat4x4 worldMatrix;
		Float4 subTexMark;
		Float4 mainCoordRect;

	protected:
		// Output vars
		Float4 position;

		// varyings
		Float4 texCoord;
		Float4 texCoordRect;
		Float4 texCoordMark;
		Float4 texCoordScale;
	};
}

