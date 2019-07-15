// DeferredCompact.h
// By PaintDream (paintdream@paintdream.com)
// 2018-4-13
//

#ifndef __DEPTHMINMAXSETUP_FS_H
#define __DEPTHMINMAXSETUP_FS_H

#include "../../../../General/Interface/IShader.h"

namespace PaintsNow {
	namespace NsSnowyStream {
		class DepthMinMaxSetupFS : public TReflected<DepthMinMaxSetupFS, IShader> {
		public:
			DepthMinMaxSetupFS();
			virtual TObject<IReflect>& operator () (IReflect& reflect) override;
			virtual String GetShaderText() override;
			BindTexture depthTexture;
			BindBuffer uniformBuffer;
			Float2 invScreenSize;

		protected:
			// inputs
			Float2 rasterCoord;

			// outputs
			Float4 outputDepth;
		};
	}
}


#endif // __DEPTHMINMAXSETUP_FS_H
