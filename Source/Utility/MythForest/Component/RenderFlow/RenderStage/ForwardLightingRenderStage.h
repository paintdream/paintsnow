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
			ForwardLightingRenderStage(const String& s);
			virtual TObject<IReflect>& operator () (IReflect& reflect) override;
			virtual void PrepareResources(Engine& engine, IRender::Queue* queue) override;
			virtual void UpdatePass(Engine& engine, IRender::Queue* queue) override;

			RenderPortLightSource LightSource;
			RenderPortCommandQueue Primitives; // input primitives

			RenderPortRenderTarget OutputColor;
		};
	}
}

#endif // __FORWARDLIGHTINGRENDERSTAGE_H__