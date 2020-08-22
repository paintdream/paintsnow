// FrameBarrierRenderStage.h
// PaintDream (paintdream@paintdream.com)
// 2018-9-11
//

#pragma once
#include "../RenderStage.h"
#include "../RenderPort/RenderPortTextureInput.h"
#include "../RenderPort/RenderPortRenderTarget.h"

namespace PaintsNow {
	class FrameBarrierRenderStage : public TReflected<FrameBarrierRenderStage, RenderStage> {
	public:
		FrameBarrierRenderStage(const String& s);

		virtual void SetMainResolution(Engine& engine, IRender::Queue* queue, uint32_t width, uint32_t height) override;
		virtual void PrepareResources(Engine& engine, IRender::Queue* queue) override;
		virtual void UpdatePass(Engine& engine, IRender::Queue* queue) override;
		virtual void Tick(Engine& engine, IRender::Queue* queue) override;
		virtual void Commit(Engine& engine, std::vector<FencedRenderQueue*>& queues, std::vector<IRender::Queue*>& instantQueues, IRender::Queue* instantQueue) override;
		virtual TObject<IReflect>& operator () (IReflect& reflect) override;

		RenderPortTextureInput Front;
		RenderPortRenderTarget Next;
	};
}

