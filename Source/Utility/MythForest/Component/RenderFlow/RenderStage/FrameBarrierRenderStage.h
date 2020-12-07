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

		void SetMainResolution(Engine& engine, IRender::Queue* queue, UShort2 res) override;
		void Prepare(Engine& engine, IRender::Queue* queue) override;
		void Update(Engine& engine, IRender::Queue* queue) override;
		void Tick(Engine& engine, IRender::Queue* queue) override;
		void Commit(Engine& engine, std::vector<IRender::Queue*>& queues, std::vector<IRender::Queue*>& instantQueues, std::vector<IRender::Queue*>& deletedQueues, IRender::Queue* instantQueue) override;
		TObject<IReflect>& operator () (IReflect& reflect) override;

		RenderPortTextureInput Front;
		RenderPortRenderTargetStore Next;
	};
}

