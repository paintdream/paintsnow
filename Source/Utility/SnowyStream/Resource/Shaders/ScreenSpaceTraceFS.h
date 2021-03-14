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

		BindTexture Depth;
		BindBuffer traceBuffer;

		MatrixFloat4x4 viewProjectionMatrix;
		Float3 invScreenSize;
		Float3 worldPosition;
		Float3 traceDirection;
		Float2 rasterCoord;

		Float2 traceCoord;
	};
}
