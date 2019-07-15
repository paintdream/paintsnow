// DepthBoundingRenderStage.h
// PaintDream (paintdream@paintdream.com)
// 2018-9-11
//

#ifndef __DEPTHBOUNDINGRENDERSTAGE_H__
#define __DEPTHBOUNDINGRENDERSTAGE_H__

#include "../RenderStage.h"
#include "../RenderPort/RenderPortRenderTarget.h"
#include "../RenderPort/RenderPortTextureInput.h"
#include "../../../../SnowyStream/Resource/Passes/DepthBoundingPass.h"

namespace PaintsNow {
	namespace NsMythForest {
		class DepthBoundingRenderStage : public TReflected<DepthBoundingRenderStage, GeneralRenderStageRect<NsSnowyStream::DepthBoundingPass> > {
		public:
			DepthBoundingRenderStage(const String& config = "1");
			virtual void PrepareResources(Engine& engine) override;
			virtual void UpdatePass(Engine& engine) override;

			virtual TObject<IReflect>& operator () (IReflect& reflect) override;

			RenderPortTextureInput InputDepth;
			RenderPortRenderTarget OutputDepth;
		};
	}
}

#endif // __DEPTHBOUNDINGRENDERSTAGE_H__