// DeferredLightingBufferEncodedRenderStage.h
// PaintDream (paintdream@paintdream.com)
// 2018-9-11
//

#pragma once
#include "../RenderStage.h"
#include "../RenderPort/RenderPortRenderTarget.h"
#include "../RenderPort/RenderPortTextureInput.h"
#include "../RenderPort/RenderPortLightSource.h"
#include "../RenderPort/RenderPortCameraView.h"
#include "../../../../SnowyStream/Resource/Passes/DeferredLightingBufferEncodedPass.h"

namespace PaintsNow {
	class DeferredLightingBufferEncodedRenderStage : public TReflected<DeferredLightingBufferEncodedRenderStage, GeneralRenderStageMesh<DeferredLightingBufferEncodedPass> > {
	public:
		DeferredLightingBufferEncodedRenderStage(const String& s);
		TObject<IReflect>& operator () (IReflect& reflect) override;
		void PrepareResources(Engine& engine, IRender::Queue* queue) override;
		void UpdatePass(Engine& engine, IRender::Queue* queue) override;

		TRenderPortReference<RenderPortCameraView> CameraView;
		TRenderPortReference<RenderPortLightSource> LightSource;

		RenderPortTextureInput BaseColorOcclusion;			// Base.x, Base.y, Base.z, Metallic
		RenderPortTextureInput NormalRoughnessMetallic;		// N.x, N.y, Roughness, Occlusion
		RenderPortTextureInput Depth;
		RenderPortTextureInput LightTexture;
		RenderPortTextureInput ShadowTexture;

		RenderPortRenderTargetLoad LoadDepth;
		RenderPortRenderTargetStore OutputColor;
	};
}

