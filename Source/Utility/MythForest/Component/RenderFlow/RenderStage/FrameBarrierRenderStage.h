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

			virtual void SetMainResolution(Engine& engine, IRender::Queue* queue, uint32_t width, uint32_t height, bool resizeOnly) override;
			virtual void PrepareResources(Engine& engine, IRender::Queue* queue) override;
			virtual void UpdateRenderTarget(Engine& engine, IRender::Queue* queue, bool resizeOnly) override;
			virtual void UpdatePass(Engine& engine, IRender::Queue* queue) override;
			virtual void Tick(Engine& engine, IRender::Queue* queue) override;
			virtual TObject<IReflect>& operator () (IReflect& reflect) override;

			RenderPortTextureInput Front;
			RenderPortRenderTarget Next;
		};
	}
}

#endif // __FRAMEBARRIERRENDERSTAGE_H__