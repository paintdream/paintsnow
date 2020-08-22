// ShadowMaskRenderStage.h
// PaintDream (paintdream@paintdream.com)
// 2018-9-11
//

#pragma once
#include "../RenderStage.h"
#include "../RenderPort/RenderPortRenderTarget.h"
#include "../RenderPort/RenderPortTextureInput.h"
#include "../RenderPort/RenderPortLightSource.h"
#include "../RenderPort/RenderPortCameraView.h"
#include "../../../../SnowyStream/Resource/Passes/ShadowMaskPass.h"

namespace PaintsNow {
	class ShadowMaskRenderStage : public TReflected<ShadowMaskRenderStage, GeneralRenderStageRect<ShadowMaskPass> > {
	public:
		ShadowMaskRenderStage(const String& config = "1");
		virtual void PrepareResources(Engine& engine, IRender::Queue* queue) override;
		virtual void UpdatePass(Engine& engine, IRender::Queue* queue) override;

		virtual TObject<IReflect>& operator () (IReflect& reflect) override;

		TRenderPortReference<RenderPortLightSource> LightSource;
		TRenderPortReference<RenderPortCameraView> CameraView;
		RenderPortTextureInput InputDepth;
		RenderPortRenderTarget OutputMask;

		TShared<TextureResource> emptyShadowMask;
	};
}

