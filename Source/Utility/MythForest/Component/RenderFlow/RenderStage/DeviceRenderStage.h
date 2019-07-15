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
			virtual void PrepareResources(Engine& engine) override;
			virtual void UpdatePass(Engine& engine) override;

			virtual TObject<IReflect>& operator () (IReflect& reflect) override;

			RenderPortTextureInput InputColor;
		};
	}
}

#endif // __DEVICERENDERSTAGE_H__