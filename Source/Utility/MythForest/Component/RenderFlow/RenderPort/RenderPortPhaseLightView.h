// RenderPortPhaseLightView.h
// PaintDream (paintdream@paintdream.com)
// 2018-10-15
//

#pragma once
#include "../RenderPort.h"
#include "../../Light/LightComponent.h"
#include <queue>

namespace PaintsNow {
	class RenderPortPhaseLightView : public TReflected<RenderPortPhaseLightView, RenderPort> {
	public:
		virtual TObject<IReflect>& operator () (IReflect& reflect) override;

		RenderPortPhaseLightView();

		struct PhaseInfo {
			MatrixFloat4x4 viewProjectionMatrix;
			MatrixFloat4x4 projectionMatrix;
			TShared<TextureResource> irradiance;
			TShared<TextureResource> depth;
		};

		virtual void Initialize(IRender& render, IRender::Queue* mainQueue) override;
		virtual void Uninitialize(IRender& render, IRender::Queue* mainQueue) override;
		virtual bool UpdateDataStream(RenderPort& source) override;
		virtual bool BeginFrame(IRender& render) override;
		virtual void EndFrame(IRender& render) override;

		std::vector<PhaseInfo> phases;
	};
}

