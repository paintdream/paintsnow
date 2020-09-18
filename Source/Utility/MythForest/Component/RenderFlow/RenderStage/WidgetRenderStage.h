// WidgetRenderStage.h
// PaintDream (paintdream@paintdream.com)
// 2018-9-11
//

#pragma once
#include "../RenderStage.h"
#include "../RenderPort/RenderPortCommandQueue.h"
#include "../RenderPort/RenderPortRenderTarget.h"

namespace PaintsNow {
	class WidgetRenderStage : public TReflected<WidgetRenderStage, RenderStage> {
	public:
		WidgetRenderStage(const String& s);
		void PrepareResources(Engine& engine, IRender::Queue* queue) override;
		void UpdatePass(Engine& engine, IRender::Queue* queue) override;

		TObject<IReflect>& operator () (IReflect& reflect) override;

		RenderPortCommandQueue Widgets;
		RenderPortRenderTargetLoad InputColor; // optional
		RenderPortRenderTargetStore OutputColor;
	};
}

