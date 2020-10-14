// AntiAliasingRenderStage.h
// PaintDream (paintdream@paintdream.com)
// 2018-9-11
//

#pragma once
#include "../RenderStage.h"
#include "../RenderPort/RenderPortRenderTarget.h"
#include "../RenderPort/RenderPortTextureInput.h"
#include "../RenderPort/RenderPortCameraView.h"
#include "../../../../SnowyStream/Resource/Passes/AntiAliasingPass.h"

namespace PaintsNow {
	class AntiAliasingRenderStage : public TReflected<AntiAliasingRenderStage, GeneralRenderStageMesh<AntiAliasingPass> > {
	public:
		AntiAliasingRenderStage(const String& options);
		void PrepareResources(Engine& engine, IRender::Queue* queue) override;
		void UpdatePass(Engine& engine, IRender::Queue* queue) override;

		TObject<IReflect>& operator () (IReflect& reflect) override;

		TRenderPortReference<RenderPortCameraView> CameraView;
		RenderPortTextureInput InputColor;
		RenderPortTextureInput LastInputColor;
		RenderPortTextureInput Depth;
		RenderPortRenderTargetStore OutputColor;
	};
}

