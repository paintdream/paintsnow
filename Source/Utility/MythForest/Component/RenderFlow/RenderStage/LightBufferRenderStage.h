// LightBufferRenderStage.h
// PaintDream (paintdream@paintdream.com)
// 2018-9-11
//

#pragma once
#include "../RenderStage.h"
#include "../RenderPort/RenderPortRenderTarget.h"
#include "../RenderPort/RenderPortTextureInput.h"
#include "../RenderPort/RenderPortLightSource.h"
#include "../RenderPort/RenderPortCameraView.h"
#include "../../../../SnowyStream/Resource/Passes/LightBufferPass.h"

namespace PaintsNow {
	class LightBufferRenderStage : public TReflected<LightBufferRenderStage, GeneralRenderStageRect<LightBufferPass> > {
	public:
		LightBufferRenderStage(const String& config = "1");
		virtual void PrepareResources(Engine& engine, IRender::Queue* queue) override;
		virtual void UpdatePass(Engine& engine, IRender::Queue* queue) override;
		virtual void Uninitialize(Engine& engine, IRender::Queue* queue) override;

		virtual TObject<IReflect>& operator () (IReflect& reflect) override;

		TRenderPortReference<RenderPortCameraView> CameraView;
		RenderPortLightSource LightSource;
		RenderPortTextureInput InputDepth;
		RenderPortRenderTarget LightTexture;
	};
}

