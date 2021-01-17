// ForwardLightingRenderStage.h
// PaintDream (paintdream@paintdream.com)
// 2018-9-11
//

#pragma once
#include "../RenderStage.h"
#include "../RenderPort/RenderPortRenderTarget.h"
#include "../RenderPort/RenderPortCommandQueue.h"
#include "../RenderPort/RenderPortLightSource.h"
#include "../../../../SnowyStream/Resource/Passes/ForwardLightingPass.h"

namespace PaintsNow {
	class ForwardLightingRenderStage : public TReflected<ForwardLightingRenderStage, RenderStage> {
	public:
		ForwardLightingRenderStage(const String& s);
		TObject<IReflect>& operator () (IReflect& reflect) override;
		void Prepare(Engine& engine, IRender::Queue* queue) override;
		void Update(Engine& engine, IRender::Queue* queue) override;

		RenderPortLightSource LightSource;
		RenderPortCommandQueue Primitives; // input primitives

		RenderPortRenderTargetLoad InputColor;
		RenderPortRenderTargetLoad InputDepth;
		RenderPortRenderTargetStore OutputColor;
		RenderPortRenderTargetStore OutputDepth;
	};
}

