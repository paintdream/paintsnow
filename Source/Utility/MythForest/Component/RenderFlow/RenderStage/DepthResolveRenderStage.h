// DepthResolveRenderStage.h
// PaintDream (paintdream@paintdream.com)
// 2018-9-11
//

#pragma once
#include "../RenderStage.h"
#include "../RenderPort/RenderPortRenderTarget.h"
#include "../RenderPort/RenderPortTextureInput.h"
#include "../../../../SnowyStream/Resource/Passes/DepthResolvePass.h"

namespace PaintsNow {
	class DepthResolveRenderStage : public TReflected<DepthResolveRenderStage, GeneralRenderStageMesh<DepthResolvePass> > {
	public:
		DepthResolveRenderStage(const String& config = "1");
		void Prepare(Engine& engine, IRender::Queue* queue) override;
		void Update(Engine& engine, IRender::Queue* queue) override;

		TObject<IReflect>& operator () (IReflect& reflect) override;

		RenderPortTextureInput InputDepth;
		RenderPortRenderTargetStore OutputDepth;
	};
}

