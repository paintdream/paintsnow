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
		void SetMainResolution(Engine& engine, IRender::Queue* queue, uint32_t width, uint32_t height) override;
		void PrepareResources(Engine& engine, IRender::Queue* queue) override;
		void UpdatePass(Engine& engine, IRender::Queue* queue) override;
		void Tick(Engine& engine, IRender::Queue* queue) override;
		void Commit(Engine& engine, std::vector<IRender::Queue*>& queues, std::vector<IRender::Queue*>& instantQueues, std::vector<IRender::Queue*>& deletedQueues, IRender::Queue* instantQueue) override;

		TObject<IReflect>& operator () (IReflect& reflect) override;

		RenderPortTextureInput InputColor;
	};
}

