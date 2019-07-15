// WidgetRenderStage.h
// PaintDream (paintdream@paintdream.com)
// 2018-9-11
//

#ifndef __WIDGETRENDERSTAGE_H__
#define __WIDGETRENDERSTAGE_H__

#include "../RenderStage.h"
#include "../RenderPort/RenderPortCommandQueue.h"
#include "../RenderPort/RenderPortRenderTarget.h"

namespace PaintsNow {
	namespace NsMythForest {
		class WidgetRenderStage : public TReflected<WidgetRenderStage, RenderStage> {
		public:
			WidgetRenderStage();
			virtual void PrepareResources(Engine& engine) override;
			virtual void UpdatePass(Engine& engine) override;

			virtual TObject<IReflect>& operator () (IReflect& reflect) override;

			RenderPortCommandQueue Widgets;
			RenderPortRenderTarget OutputColor;
		};
	}
}

#endif // __WIDGETRENDERSTAGE_H__