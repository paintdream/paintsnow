// StencilMaskRenderStage.h
// PaintDream (paintdream@paintdream.com)
// 2018-9-11
//

#pragma once
#include "../RenderStage.h"
#include "../RenderPort/RenderPortRenderTarget.h"
#include "../RenderPort/RenderPortTextureInput.h"
#include "../RenderPort/RenderPortCameraView.h"
#include "../../../../SnowyStream/Resource/Passes/ConstMapPass.h"

namespace PaintsNow {
	class StencilMaskRenderStage : public TReflected<StencilMaskRenderStage, GeneralRenderStageRect<ConstMapPass> > {
	public:
		StencilMaskRenderStage(const String& config = "1");
		void PrepareResources(Engine& engine, IRender::Queue* queue) override;
		void UpdatePass(Engine& engine, IRender::Queue* queue) override;

		TObject<IReflect>& operator () (IReflect& reflect) override;

		RenderPortRenderTargetLoad InputDepth;
		RenderPortRenderTargetLoad InputColorPlaceHolder;
		RenderPortRenderTargetStore OutputDepth;
	};
}

