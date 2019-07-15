// DeferredCompact.h
// By PaintDream (paintdream@paintdream.com)
// 2018-4-13
//

#ifndef __ANTIALISING_FS_H
#define __ANTIALISING_FS_H

#include "../../../../General/Interface/IShader.h"

namespace PaintsNow {
	namespace NsSnowyStream {
		class AntiAliasingFS : public TReflected<AntiAliasingFS, IShader> {
		public:
			AntiAliasingFS();
			virtual TObject<IReflect>& operator () (IReflect& reflect) override;
			virtual String GetShaderText() override;
			BindTexture inputTexture;
			BindTexture lastInputTexture;
			BindTexture depthTexture;
			BindBuffer uniformBuffer;

			MatrixFloat4x4 reprojectionMatrix;
			Float2 invScreenSize;
			Float2 unjitter;
			float lastRatio;

		protected:
			// inputs
			Float2 rasterCoord;

			// outputs
			Float4 outputColor;
		};
	}
}


#endif // __ANTIALISING_FS_H
