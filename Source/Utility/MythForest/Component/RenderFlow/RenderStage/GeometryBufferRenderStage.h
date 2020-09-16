// GeometryBufferRenderStage.h
// PaintDream (paintdream@paintdream.com)
// 2018-9-11
//

#pragma once
#include "../RenderStage.h"
#include "../RenderPort/RenderPortCommandQueue.h"
#include "../RenderPort/RenderPortRenderTarget.h"
#include "../RenderPort/RenderPortCameraView.h"

namespace PaintsNow {
	class GeometryBufferRenderStage : public TReflected<GeometryBufferRenderStage, RenderStage> {
	public:
		GeometryBufferRenderStage(const String& s);

		TObject<IReflect>& operator () (IReflect& reflect) override;
		void PrepareResources(Engine& engine, IRender::Queue* queue) override;

		RenderPortCameraView CameraView;
		RenderPortCommandQueue Primitives;		// input primitives

		RenderPortRenderTargetStore BaseColorOcclusion;
		RenderPortRenderTargetStore NormalRoughnessMetallic;
		RenderPortRenderTargetStore Depth;
	};
}

