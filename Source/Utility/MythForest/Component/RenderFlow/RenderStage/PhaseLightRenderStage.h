// PhaseLightRenderStage.h
// PaintDream (paintdream@paintdream.com)
// 2018-9-11
//

#ifndef __PHASELIGHTRENDERSTAGE_H__
#define __PHASELIGHTRENDERSTAGE_H__

#include "../RenderStage.h"
#include "../RenderPort/RenderPortRenderTarget.h"
#include "../RenderPort/RenderPortTextureInput.h"
#include "../RenderPort/RenderPortPhaseLightView.h"
#include "../RenderPort/RenderPortCameraView.h"
#include "../../../../SnowyStream/Resource/Passes/MultiHashTracePass.h"

namespace PaintsNow {
	namespace NsMythForest {
		class PhaseLightRenderStage : public TReflected<PhaseLightRenderStage, GeneralRenderStageRect<NsSnowyStream::MultiHashTracePass> > {
		public:
			PhaseLightRenderStage();
			virtual void PrepareResources(Engine& engine, IRender::Queue* queue) override;
			virtual void UpdatePass(Engine& engine, IRender::Queue* queue) override;
			virtual void Tick(Engine& engine, IRender::Queue* queue) override;

			virtual TObject<IReflect>& operator () (IReflect& reflect) override;

			TRenderPortReference<RenderPortCameraView> CameraView;
			RenderPortTextureInput InputColor;
			RenderPortTextureInput Depth;
			RenderPortTextureInput BaseColorOcclusion;
			RenderPortTextureInput NormalRoughnessMetallic;
			RenderPortPhaseLightView PhaseLightView;
			RenderPortRenderTarget OutputColor;
		};
	}
}

#endif // __PHASELIGHTRENDERSTAGE_H__