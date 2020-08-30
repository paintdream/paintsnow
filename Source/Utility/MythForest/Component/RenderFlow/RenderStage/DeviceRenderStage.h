// DeviceRenderStage.h
// PaintDream (paintdream@paintdream.com)
// 2018-9-11
//

#pragma once
#include "../RenderStage.h"
#include "../RenderPort/RenderPortTextureInput.h"

namespace PaintsNow {
	class DeviceRenderStage : public TReflected<DeviceRenderStage, RenderStage> {
	public:
		DeviceRenderStage(const String& config = "1");
		virtual void SetMainResolution(Engine& engine, IRender::Queue* queue, uint32_t width, uint32_t height) override;
		virtual void PrepareResources(Engine& engine, IRender::Queue* queue) override;
		virtual void UpdatePass(Engine& engine, IRender::Queue* queue) override;
		virtual void Tick(Engine& engine, IRender::Queue* queue) override;
		virtual void Commit(Engine& engine, std::vector<IRender::Queue*>& queues, std::vector<IRender::Queue*>& instantQueues, std::vector<IRender::Queue*>& deletedQueues, IRender::Queue* instantQueue) override;

		virtual TObject<IReflect>& operator () (IReflect& reflect) override;

		RenderPortTextureInput InputColor;
	};
}

