// AntiAliasingRenderStage.h
// PaintDream (paintdream@paintdream.com)
// 2018-9-11
//

#ifndef __ANTIALIASINGRENDERSTAGE_H__
#define __ANTIALIASINGRENDERSTAGE_H__

#include "../RenderStage.h"
#include "../RenderPort/RenderPortRenderTarget.h"
#include "../RenderPort/RenderPortTextureInput.h"
#include "../RenderPort/RenderPortCameraView.h"
#include "../../../../SnowyStream/Resource/Passes/AntiAliasingPass.h"

namespace PaintsNow {
	namespace NsMythForest {
		class AntiAliasingRenderStage : public TReflected<AntiAliasingRenderStage, GeneralRenderStageRect<NsSnowyStream::AntiAliasingPass> > {
		public:
			AntiAliasingRenderStage(const String& options);
			virtual void PrepareResources(Engine& engine, IRender::Queue* queue) override;
			virtual void UpdatePass(Engine& engine, IRender::Queue* queue) override;

			virtual TObject<IReflect>& operator () (IReflect& reflect) override;

			TRenderPortReference<RenderPortCameraView> CameraView;
			RenderPortTextureInput InputColor;
			RenderPortTextureInput LastInputColor;
			RenderPortTextureInput Depth;
			RenderPortRenderTarget OutputColor;
		};
	}
}

#endif // __ANTIALIASINGRENDERSTAGE_H__