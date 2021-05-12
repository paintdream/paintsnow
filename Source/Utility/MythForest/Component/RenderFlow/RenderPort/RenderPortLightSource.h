// RenderPortLightSource.h
// PaintDream (paintdream@paintdream.com)
// 2018-10-15
//

#pragma once
#include "../RenderPort.h"
#include "../../Light/LightComponent.h"

namespace PaintsNow {
	class RenderPortLightSource : public TReflected<RenderPortLightSource, RenderPort> {
	public:
		TObject<IReflect>& operator () (IReflect& reflect) override;

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
			float cubeStrength;
		};

		RenderPortLightSource();

		void Initialize(IRender& render, IRender::Queue* mainQueue) override;
		void Uninitialize(IRender& render, IRender::Queue* mainQueue) override;
		bool BeginFrame(IRender& render) override;
		void EndFrame(IRender& render) override;

		std::vector<LightElement> lightElements;
		TShared<TextureResource> cubeMapTexture;
		TShared<TextureResource> skyMapTexture;
		float cubeStrength;
		uint8_t stencilMask;
		uint8_t stencilShadow;
		uint8_t reserved[2];
	};
}

