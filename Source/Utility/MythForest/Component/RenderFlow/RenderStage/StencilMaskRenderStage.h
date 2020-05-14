// StencilMaskRenderStage.h
// PaintDream (paintdream@paintdream.com)
// 2018-9-11
//

#ifndef __STENCILMASKRENDERSTAGE_H__
#define __STENCILMASKRENDERSTAGE_H__

#include "../RenderStage.h"
#include "../RenderPort/RenderPortRenderTarget.h"
#include "../RenderPort/RenderPortTextureInput.h"
#include "../RenderPort/RenderPortLoadTarget.h"
#include "../RenderPort/RenderPortCameraView.h"
#include "../../../../SnowyStream/Resource/Passes/ConstMapPass.h"

namespace PaintsNow {
	namespace NsMythForest {
		class StencilMaskRenderStage : public TReflected<StencilMaskRenderStage, GeneralRenderStageRect<NsSnowyStream::ConstMapPass> > {
		public:
			StencilMaskRenderStage(const String& config = "1");
			virtual void PrepareResources(Engine& engine, IRender::Queue* queue) override;
			virtual void UpdatePass(Engine& engine, IRender::Queue* queue) override;

			virtual TObject<IReflect>& operator () (IReflect& reflect) override;

			RenderPortLoadTarget InputDepth;
			RenderPortLoadTarget InputColorPlaceHolder;
			RenderPortRenderTarget OutputDepth;
		};
	}
}

#endif // __STENCILMASKRENDERSTAGE_H__