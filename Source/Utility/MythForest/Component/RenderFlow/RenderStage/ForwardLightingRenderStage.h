// ForwardLightingRenderStage.h
// PaintDream (paintdream@paintdream.com)
// 2018-9-11
//

#ifndef __FORWARDLIGHTINGRENDERSTAGE_H__
#define __FORWARDLIGHTINGRENDERSTAGE_H__

#include "../RenderStage.h"
#include "../RenderPort/RenderPortRenderTarget.h"
#include "../RenderPort/RenderPortCommandQueue.h"
#include "../RenderPort/RenderPortLightSource.h"
#include "../../../../SnowyStream/Resource/Passes/ForwardLightingPass.h"

namespace PaintsNow {
	namespace NsMythForest {
		class ForwardLightingRenderStage : public TReflected<ForwardLightingRenderStage, RenderStage> {
		public:
			ForwardLightingRenderStage();
			virtual TObject<IReflect>& operator () (IReflect& reflect) override;
			virtual void PrepareResources(Engine& engine) override;
			virtual void UpdatePass(Engine& engine) override;

			RenderPortLightSource LightSource;
			RenderPortCommandQueue Primitives; // input primitives

			RenderPortRenderTarget OutputColor;
		};
	}
}

#endif // __FORWARDLIGHTINGRENDERSTAGE_H__