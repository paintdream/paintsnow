// ShadowMaskRenderStage.h
// PaintDream (paintdream@paintdream.com)
// 2018-9-11
//

#ifndef __SHADOWMASKRENDERSTAGE_H__
#define __SHADOWMASKRENDERSTAGE_H__

#include "../RenderStage.h"
#include "../RenderPort/RenderPortRenderTarget.h"
#include "../RenderPort/RenderPortTextureInput.h"
#include "../RenderPort/RenderPortLightSource.h"
#include "../../../../SnowyStream/Resource/Passes/ShadowMaskPass.h"

namespace PaintsNow {
	namespace NsMythForest {
		class ShadowMaskRenderStage : public TReflected<ShadowMaskRenderStage, GeneralRenderStageRect<NsSnowyStream::ShadowMaskPass> > {
		public:
			ShadowMaskRenderStage(const String& config = "1");
			virtual void PrepareResources(Engine& engine) override;
			virtual void UpdatePass(Engine& engine) override;

			virtual TObject<IReflect>& operator () (IReflect& reflect) override;

			TRenderPortReference<RenderPortLightSource> LightSource;
			RenderPortTextureInput InputDepth;
			RenderPortRenderTarget OutputMask;
		};
	}
}

#endif // __SHADOWMASKRENDERSTAGE_H__