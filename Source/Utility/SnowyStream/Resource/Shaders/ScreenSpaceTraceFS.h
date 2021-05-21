// ScreenSpaceTraceFS.h
// PaintDream (paintdream@paintdream.com)
// 2018-4-13
//

#pragma once
#include "../../../../General/Interface/IShader.h"

namespace PaintsNow {
	class ScreenSpaceTraceFS : public TReflected<ScreenSpaceTraceFS, IShader> {
	public:
		ScreenSpaceTraceFS();
		TObject<IReflect>& operator () (IReflect& reflect) override;
		String GetShaderText() override;

		BindTexture depthTexture;
		BindTexture normalTexture;
		BindBuffer traceBuffer;

		MatrixFloat4x4 projectionMatrix;
		MatrixFloat4x4 inverseProjectionMatrix;
		Float2 invScreenSize;
		Float4 rasterCoord;

		Float4 traceCoord;
	};
}
