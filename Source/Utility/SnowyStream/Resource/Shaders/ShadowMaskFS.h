// ShadowMaskFS.h
// By PaintDream (paintdream@paintdream.com)
// 2018-4-13
//

#pragma once
#include "../../../../General/Interface/IShader.h"

namespace PaintsNow {
	class ShadowMaskFS : public TReflected<ShadowMaskFS, IShader> {
	public:
		ShadowMaskFS();
		TObject<IReflect>& operator () (IReflect& reflect) override;
		String GetShaderText() override;

		BindTexture shadowTexture0;
		BindTexture shadowTexture1;
		BindTexture shadowTexture2;
		BindTexture depthTexture;
		BindBuffer uniformBuffer;

		// Uniforms
		MatrixFloat4x4 reprojectionMatrix0;
		MatrixFloat4x4 reprojectionMatrix1;
		MatrixFloat4x4 reprojectionMatrix2;

		// Inputs
		Float2 rasterCoord;

		// Outputs
		Float4 shadow;
	};
}


