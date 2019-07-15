// RenderPortCameraView.h
// PaintDream (paintdream@paintdream.com)
// 2018-10-15
//

#ifndef __RENDERPORTCAMERAVIEW_H__
#define __RENDERPORTCAMERAVIEW_H__

#include "../RenderPort.h"
#include "../../Light/LightComponent.h"
#include <queue>

namespace PaintsNow {
	namespace NsMythForest {
		class RenderPortCameraView : public TReflected<RenderPortCameraView, RenderPort> {
		public:
			MatrixFloat4x4 projectionMatrix;
			MatrixFloat4x4 inverseProjectionMatrix;
			MatrixFloat4x4 reprojectionMatrix;
			MatrixFloat4x4 viewMatrix;
			MatrixFloat4x4 inverseViewMatrix;
			Float2 jitterOffset;
		};
	}
}

#endif // __RENDERPORTCAMERAVIEW_H__