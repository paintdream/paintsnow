// DepthBoundingRenderStage.h
// PaintDream (paintdream@paintdream.com)
// 2018-9-11
//

#pragma once
#include "../RenderStage.h"
#include "../RenderPort/RenderPortRenderTarget.h"
#include "../RenderPort/RenderPortTextureInput.h"
#include "../../../../SnowyStream/Resource/Passes/DepthBoundingPass.h"

namespace PaintsNow {
	class DepthBoundingRenderStage : public TReflected<DepthBoundingRenderStage, GeneralRenderStageRect<DepthBoundingPass> > {
	public:
		DepthBoundingRenderStage(const String& config = "1");
		void PrepareResources(Engine& engine, IRender::Queue* queue) override;
		void UpdatePass(Engine& engine, IRender::Queue* queue) override;

		TObject<IReflect>& operator () (IReflect& reflect) override;

		RenderPortTextureInput InputDepth;
		RenderPortRenderTargetStore OutputDepth;
	};
}

