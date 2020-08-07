// DeferredLightingRenderStage.h
// PaintDream (paintdream@paintdream.com)
// 2018-9-11
//

#ifndef __DEFERREDLIGHTINGRENDERSTAGE_H__
#define __DEFERREDLIGHTINGRENDERSTAGE_H__

#include "../RenderStage.h"
#include "../RenderPort/RenderPortRenderTarget.h"
#include "../RenderPort/RenderPortTextureInput.h"
#include "../RenderPort/RenderPortLightSource.h"
#include "../RenderPort/RenderPortLoadTarget.h"
#include "../RenderPort/RenderPortCameraView.h"
#include "../../../../SnowyStream/Resource/Passes/DeferredLightingPass.h"

namespace PaintsNow {
	namespace NsMythForest {
		class DeferredLightingRenderStage : public TReflected<DeferredLightingRenderStage, GeneralRenderStageRect<NsSnowyStream::DeferredLightingPass> > {
		public:
			DeferredLightingRenderStage(const String& s);
			virtual TObject<IReflect>& operator () (IReflect& reflect) override;
			virtual void PrepareResources(Engine& engine, IRender::Queue* queue) override;
			virtual void UpdatePass(Engine& engine, IRender::Queue* queue) override;

			TRenderPortReference<RenderPortCameraView> CameraView;
			TRenderPortReference<RenderPortLightSource> LightSource;

			RenderPortTextureInput BaseColorOcclusion;			// Base.x, Base.y, Base.z, Metallic
			RenderPortTextureInput NormalRoughnessMetallic;		// N.x, N.y, Roughness, Occlusion
			RenderPortTextureInput Depth;
			RenderPortTextureInput LightTexture;
			RenderPortTextureInput ShadowTexture;

			RenderPortLoadTarget LoadDepth;
			RenderPortRenderTarget OutputColor;
		};
	}
}

#endif // __DEFERREDLIGHTINGRENDERSTAGE_H__