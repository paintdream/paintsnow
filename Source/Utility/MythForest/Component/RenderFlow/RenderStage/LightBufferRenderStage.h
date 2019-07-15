// LightBufferRenderStage.h
// PaintDream (paintdream@paintdream.com)
// 2018-9-11
//

#ifndef __LIGHTBUFFERRENDERSTAGE_H__
#define __LIGHTBUFFERRENDERSTAGE_H__

#include "../RenderStage.h"
#include "../RenderPort/RenderPortRenderTarget.h"
#include "../RenderPort/RenderPortTextureInput.h"
#include "../RenderPort/RenderPortLightSource.h"
#include "../RenderPort/RenderPortCameraView.h"
#include "../../../../SnowyStream/Resource/Passes/LightBufferPass.h"

namespace PaintsNow {
	namespace NsMythForest {
		class LightBufferRenderStage : public TReflected<LightBufferRenderStage, GeneralRenderStageRect<NsSnowyStream::LightBufferPass> > {
		public:
			LightBufferRenderStage(const String& config = "1");
			virtual void PrepareResources(Engine& engine) override;
			virtual void UpdatePass(Engine& engine) override;
			virtual void Uninitialize(Engine& engine) override;

			virtual TObject<IReflect>& operator () (IReflect& reflect) override;
		
			TRenderPortReference<RenderPortCameraView> CameraView;
			RenderPortLightSource LightSource;
			RenderPortTextureInput InputDepth;
			RenderPortRenderTarget LightTexture;
		};
	}
}

#endif // __LIGHTBUFFERRENDERSTAGE_H__