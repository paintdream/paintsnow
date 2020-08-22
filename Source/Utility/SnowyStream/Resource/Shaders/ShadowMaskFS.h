// ShadowMaskFS.h
// By PaintDream (paintdream@paintdream.com)
// 2018-4-13
//

#ifndef __SHADOWMASK_FS_H
#define __SHADOWMASK_FS_H

#include "../../../../General/Interface/IShader.h"

namespace PaintsNow {
	class ShadowMaskFS : public TReflected<ShadowMaskFS, IShader> {
	public:
		ShadowMaskFS();
		virtual TObject<IReflect>& operator () (IReflect& reflect) override;
		virtual String GetShaderText();

		BindTexture shadowTexture;
		BindTexture depthTexture;
		BindBuffer uniformBuffer;

		// Uniforms
		MatrixFloat4x4 reprojectionMatrix;

		// Inputs
		Float2 rasterCoord;

		// Outputs
		Float4 shadow;
	};
}

#endif // __SHADOWMASK_FS_H
