// DeviceRenderStage.h
// PaintDream (paintdream@paintdream.com)
// 2018-9-11
//

#ifndef __DEVICERENDERSTAGE_H__
#define __DEVICERENDERSTAGE_H__

#include "../RenderStage.h"
#include "../RenderPort/RenderPortTextureInput.h"

namespace PaintsNow {
	namespace NsMythForest {
		class DeviceRenderStage : public TReflected<DeviceRenderStage, RenderStage> {
		public:
			DeviceRenderStage(const String& config = "1");
			virtual void SetMainResolution(Engine& engine, IRender::Queue* queue, uint32_t width, uint32_t height) override;
			virtual void PrepareResources(Engine& engine, IRender::Queue* queue) override;
			virtual void UpdatePass(Engine& engine, IRender::Queue* queue) override;
			virtual void Tick(Engine& engine, IRender::Queue* queue) override;
			virtual void Commit(Engine& engine, std::vector<FencedRenderQueue*>& queues, IRender::Queue* instantQueue) override;

			virtual TObject<IReflect>& operator () (IReflect& reflect) override;

			RenderPortTextureInput InputColor;
		};
	}
}

#endif // __DEVICERENDERSTAGE_H__