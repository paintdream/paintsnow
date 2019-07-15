// MultiHashTraceFS.h
// By PaintDream (paintdream@paintdream.com)
// 2018-4-13
//

#ifndef __MULTIHASHTRACE_FS_H
#define __MULTIHASHTRACE_FS_H

#include "../../../../General/Interface/IShader.h"

namespace PaintsNow {
	namespace NsSnowyStream {
		class MultiHashTraceFS : public TReflected<MultiHashTraceFS, IShader> {
		public:
			MultiHashTraceFS();
			virtual TObject<IReflect>& operator () (IReflect& reflect) override;
			virtual String GetShaderText() override;

			BindTexture srcDepthTexture;
			BindTexture srcLitTexture;
			BindTexture dstDepthTexture;
			BindTexture dstBaseColorOcclusionTexture;
			BindTexture dstNormalRoughnessMetallicTexture;

			BindBuffer uniformBuffer;

			// Inputs
			Float2 rasterCoord;

			// Uniforms
			MatrixFloat4x4 dstInverseProjection;
			MatrixFloat4x4 srcProjection;
			MatrixFloat4x4 srcInverseProjection;
			std::vector<Float2> offsets;
			Float3 srcOrigin;
			float sigma;

			// Outputs
			Float4 dstLit;
		};
	}
}


#endif // __MULTIHASHTRACE_FS_H
