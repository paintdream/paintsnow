// DeferredCompact.h
// By PaintDream (paintdream@paintdream.com)
// 2018-4-13
//

#ifndef __DEPTHRESOLVE_FS_H
#define __DEPTHRESOLVE_FS_H

#include "../../../../General/Interface/IShader.h"

namespace PaintsNow {
	class DepthResolveFS : public TReflected<DepthResolveFS, IShader> {
	public:
		DepthResolveFS();
		TObject<IReflect>& operator () (IReflect& reflect) override;
		String GetShaderText() override;
		BindTexture depthTexture;
		BindBuffer uniformBuffer;
		Float4 resolveParam;

	protected:
		// inputs
		Float2 rasterCoord;

		// outputs
		Float4 outputDepth;
	};
}

#endif // __DEPTHRESOLVE_FS_H
