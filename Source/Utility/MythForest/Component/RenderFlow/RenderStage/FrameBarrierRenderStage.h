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

			virtual void SetMainResolution(Engine& engine, IRender::Queue* queue, uint32_t width, uint32_t height) override;
			virtual void PrepareResources(Engine& engine, IRender::Queue* queue) override;
			virtual void UpdatePass(Engine& engine, IRender::Queue* queue) override;
			virtual void Tick(Engine& engine, IRender::Queue* queue) override;
			virtual void Commit(Engine& engine, std::vector<ZFencedRenderQueue*>& queues, IRender::Queue* instantQueue) override;
			virtual TObject<IReflect>& operator () (IReflect& reflect) override;

			RenderPortTextureInput Front;
			RenderPortRenderTarget Next;
		};
	}
}

#endif // __FRAMEBARRIERRENDERSTAGE_H__