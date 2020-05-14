// DepthResolveRenderStage.h
// PaintDream (paintdream@paintdream.com)
// 2018-9-11
//

#ifndef __DEPTHRESOLVERENDERSTAGE_H__
#define __DEPTHRESOLVERENDERSTAGE_H__

#include "../RenderStage.h"
#include "../RenderPort/RenderPortRenderTarget.h"
#include "../RenderPort/RenderPortTextureInput.h"
#include "../../../../SnowyStream/Resource/Passes/DepthResolvePass.h"

namespace PaintsNow {
	namespace NsMythForest {
		class DepthResolveRenderStage : public TReflected<DepthResolveRenderStage, GeneralRenderStageRect<NsSnowyStream::DepthResolvePass> > {
		public:
			DepthResolveRenderStage(const String& config = "1");
			virtual void PrepareResources(Engine& engine, IRender::Queue* queue) override;
			virtual void UpdatePass(Engine& engine, IRender::Queue* queue) override;

			virtual TObject<IReflect>& operator () (IReflect& reflect) override;

			RenderPortTextureInput InputDepth;
			RenderPortRenderTarget OutputDepth;
		};
	}
}

#endif // __DEPTHRESOLVERENDERSTAGE_H__