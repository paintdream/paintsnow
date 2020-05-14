// GeometryBufferRenderStage.h
// PaintDream (paintdream@paintdream.com)
// 2018-9-11
//

#ifndef __GEOMETRYBUFFERRENDERSTAGE_H__
#define __GEOMETRYBUFFERRENDERSTAGE_H__

#include "../RenderStage.h"
#include "../RenderPort/RenderPortCommandQueue.h"
#include "../RenderPort/RenderPortRenderTarget.h"
#include "../RenderPort/RenderPortCameraView.h"

namespace PaintsNow {
	namespace NsMythForest {
		class GeometryBufferRenderStage : public TReflected<GeometryBufferRenderStage, RenderStage> {
		public:
			GeometryBufferRenderStage();

			virtual TObject<IReflect>& operator () (IReflect& reflect) override;
			virtual void PrepareResources(Engine& engine, IRender::Queue* queue) override;

			RenderPortCameraView CameraView;
			RenderPortCommandQueue Primitives;		// input primitives

			RenderPortRenderTarget BaseColorOcclusion;
			RenderPortRenderTarget NormalRoughnessMetallic;
			RenderPortRenderTarget Depth;
		};
	}
}

#endif // __GEOMETRYBUFFERRENDERSTAGE_H__