// ScreenSpaceTraceFS.h
// By PaintDream (paintdream@paintdream.com)
// 2018-4-13
//

#ifndef __SCREENSPACETRACE_FS_H
#define __SCREENSPACETRACE_FS_H

#include "../../../../General/Interface/IShader.h"

namespace PaintsNow {
	namespace NsSnowyStream {
		class ScreenSpaceTraceFS : public TReflected<ScreenSpaceTraceFS, IShader> {
		public:
			ScreenSpaceTraceFS();
			virtual TObject<IReflect>& operator () (IReflect& reflect) override;
			virtual String GetShaderText() override;

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
}


#endif // __SCREENSPACETRACE_FS_H
