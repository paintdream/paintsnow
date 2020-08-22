// MultiHashLightFS.h
// By PaintDream (paintdream@paintdream.com)
// 2018-4-13
//

#ifndef __MULTIHASHLIGHT_FS_H
#define __MULTIHASHLIGHT_FS_H

#include "../../../../General/Interface/IShader.h"

namespace PaintsNow {
	class MultiHashLightFS : public TReflected<MultiHashLightFS, IShader> {
	public:
		MultiHashLightFS();
		virtual TObject<IReflect>& operator () (IReflect& reflect) override;
		virtual String GetShaderText() override;

		BindTexture depthTexture;
		BindTexture lightDepthTexture;
		BindBuffer lightParamBuffer;

		Float2 rasterCoord;
		MatrixFloat4x4 invProjectionMatrix;
		MatrixFloat4x4 lightProjectionMatrix;
		Float3 lightColor;
		float lightAttenuation;

		// output
		Float4 blendColor;
	};
}

#endif // __MULTIHASHLIGHT_FS_H
