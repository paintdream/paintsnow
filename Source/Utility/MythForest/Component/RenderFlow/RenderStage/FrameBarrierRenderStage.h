// FrameBarrierRenderStage.h
// PaintDream (paintdream@paintdream.com)
// 2018-9-11
//

#ifndef __FRAMEBARRIERRENDERSTAGE_H__
#define __FRAMEBARRIERRENDERSTAGE_H__

#include "../RenderStage.h"
#include "../RenderPort/RenderPortTextureInput.h"
#include "../RenderPort/RenderPortRenderTarget.h"

namespace PaintsNow {
	namespace NsMythForest {
		class FrameBarrierRenderStage : public TReflected<FrameBarrierRenderStage, RenderStage> {
		public:
			FrameBarrierRenderStage();

			virtual void SetMainResolution(Engine& engine, uint32_t width, uint32_t height);
			virtual void PrepareResources(Engine& engine);
			virtual void UpdateRenderTarget(Engine& engine);
			virtual void UpdatePass(Engine& engine);
			virtual void UpdateComplete(Engine& engine);
			virtual void PrepareRenderQueues(Engine& engine, std::vector<ZRenderQueue*>& queues) override;
			virtual TObject<IReflect>& operator () (IReflect& reflect) override;

			RenderPortTextureInput Front;
			RenderPortRenderTarget Next;
		};
	}
}

#endif // __FRAMEBARRIERRENDERSTAGE_H__