// DepthBoundingSetupRenderStage.h
// PaintDream (paintdream@paintdream.com)
// 2018-9-11
//

#pragma once
#include "../RenderStage.h"
#include "../RenderPort/RenderPortRenderTarget.h"
#include "../RenderPort/RenderPortTextureInput.h"
#include "../../../../SnowyStream/Resource/Passes/DepthBoundingSetupPass.h"

namespace PaintsNow {
	class DepthBoundingSetupRenderStage : public TReflected<DepthBoundingSetupRenderStage, GeneralRenderStageRect<DepthBoundingSetupPass> > {
	public:
		DepthBoundingSetupRenderStage(const String& config = "1");
		virtual void PrepareResources(Engine& engine, IRender::Queue* queue) override;
		virtual void UpdatePass(Engine& engine, IRender::Queue* queue) override;

		virtual TObject<IReflect>& operator () (IReflect& reflect) override;

		RenderPortTextureInput InputDepth;
		RenderPortRenderTarget OutputDepth;
	};
}

