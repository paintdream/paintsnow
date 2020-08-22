// DeferredCompact.h
// By PaintDream (paintdream@paintdream.com)
// 2018-4-13
//

#ifndef __DEPTHMINMAX_FS_H
#define __DEPTHMINMAX_FS_H

#include "../../../../General/Interface/IShader.h"

namespace PaintsNow {
	class DepthMinMaxFS : public TReflected<DepthMinMaxFS, IShader> {
	public:
		DepthMinMaxFS();
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

#endif // __DEPTHMINMAX_FS_H
