// RenderPortLightSource.h
// PaintDream (paintdream@paintdream.com)
// 2018-10-15
//

#pragma once
#include "../RenderPort.h"
#include "../../Light/LightComponent.h"
#include <queue>

namespace PaintsNow {
	class RenderPortLightSource : public TReflected<RenderPortLightSource, RenderPort> {
	public:
		virtual TObject<IReflect>& operator () (IReflect& reflect) override;

		struct LightElement {
			Float4 position;
			Float4 colorAttenuation;

			struct Shadow {
				MatrixFloat4x4 shadowMatrix;
				TShared<TextureResource> shadowTexture;
			};

			std::vector<Shadow> shadows;
		};

		struct EnvCubeElement {
			TShared<TextureResource> cubeMapTexture;
			TShared<TextureResource> skyMapTexture;
			Float3 position;
		};

		RenderPortLightSource();

		virtual void Initialize(IRender& render, IRender::Queue* mainQueue) override;
		virtual void Uninitialize(IRender& render, IRender::Queue* mainQueue) override;
		virtual bool UpdateDataStream(RenderPort& source) override;
		virtual bool BeginFrame(IRender& render) override;
		virtual void EndFrame(IRender& render) override;

		std::vector<LightElement> lightElements;
		TShared<TextureResource> cubeMapTexture;
		TShared<TextureResource> skyMapTexture;
		uint8_t stencilMask;
		uint8_t reserved[3];
	};
}

