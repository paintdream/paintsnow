// ScreenSpaceTraceRenderStage.h
// PaintDream (paintdream@paintdream.com)
// 2018-9-11
//

#pragma once
#include "../RenderStage.h"
#include "../RenderPort/RenderPortTextureInput.h"
#include "../RenderPort/RenderPortRenderTarget.h"
#include "../RenderPort/RenderPortCameraView.h"
#include "../../../../SnowyStream/Resource/Passes/ScreenSpaceTracePass.h"

namespace PaintsNow {
	class ScreenSpaceTraceRenderStage : public TReflected<ScreenSpaceTraceRenderStage, GeneralRenderStageDraw<ScreenSpaceTracePass> > 	{
	public:
		ScreenSpaceTraceRenderStage(const String& s);

		TObject<IReflect>& operator () (IReflect& reflect) override;
		void Prepare(Engine& engine, IRender::Queue* queue) override;
		void Update(Engine& engine, IRender::Queue* queue) override;

		RenderPortTextureInput Depth;
		RenderPortTextureInput Normal;
		TRenderPortReference<RenderPortCameraView> CameraView;
		RenderPortRenderTargetStore ScreenCoord;
	};
}

