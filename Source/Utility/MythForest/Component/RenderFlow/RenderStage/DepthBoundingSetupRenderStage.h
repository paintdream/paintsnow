// DepthBoundingSetupRenderStage.h
// PaintDream (paintdream@paintdream.com)
// 2018-9-11
//

#ifndef __DEPTHBOUNDINGSETUPRENDERSTAGE_H__
#define __DEPTHBOUNDINGSETUPRENDERSTAGE_H__

#include "../RenderStage.h"
#include "../RenderPort/RenderPortRenderTarget.h"
#include "../RenderPort/RenderPortTextureInput.h"
#include "../../../../SnowyStream/Resource/Passes/DepthBoundingSetupPass.h"

namespace PaintsNow {
	namespace NsMythForest {
		class DepthBoundingSetupRenderStage : public TReflected<DepthBoundingSetupRenderStage, GeneralRenderStageRect<NsSnowyStream::DepthBoundingSetupPass> > {
		public:
			DepthBoundingSetupRenderStage(const String& config = "1");
			virtual void PrepareResources(Engine& engine) override;
			virtual void UpdatePass(Engine& engine) override;

			virtual TObject<IReflect>& operator () (IReflect& reflect) override;

			RenderPortTextureInput InputDepth;
			RenderPortRenderTarget OutputDepth;
		};
	}
}

#endif // __DEPTHBOUNDINGSETUPRENDERSTAGE_H__