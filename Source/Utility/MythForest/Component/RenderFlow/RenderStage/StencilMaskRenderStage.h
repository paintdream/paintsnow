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
	class StencilMaskRenderStage : public TReflected<StencilMaskRenderStage, GeneralRenderStageDraw<ConstMapPass> > {
	public:
		StencilMaskRenderStage(const String& config = "1");
		void Prepare(Engine& engine, IRender::Queue* queue) override;
		void Update(Engine& engine, IRender::Queue* queue) override;

		TObject<IReflect>& operator () (IReflect& reflect) override;

		RenderPortRenderTargetLoad InputDepthStencil;
		RenderPortRenderTargetStore OutputMask;
	};
}

