// SkyShadingFS.h
// PaintDream (paintdream@paintdream.com)
// 2018-4-13
//

#pragma once
#include "../../../../General/Interface/IShader.h"

namespace PaintsNow {
	// [Precomputed Atmospheric Scattering](https://hal.inria.fr/inria-00288758/en)
	// https://ebruneton.github.io/precomputed_atmospheric_scattering/demo.html

	class SkyShadingFS : public TReflected<SkyShadingFS, IShader> {
	public:
		SkyShadingFS();
		TObject<IReflect>& operator () (IReflect& reflect) override;
		String GetShaderText() override;

		IShader::BindBuffer paramBuffer;
		IShader::BindTexture transmittanceTexture;
		IShader::BindTexture abstractScatteringTexture;
		IShader::BindTexture reducedScatteringTexture;
		IShader::BindTexture scatteringTexture;
		IShader::BindTexture scatteringDensityTexture;
		IShader::BindTexture irradianceTexture;

		float exposure;
		Float3 whitePoint;
		Float3 earthCenter;
		Float3 sunDirection;
		Float3 sunSize;
		Float3 sphereCenter;
		Float3 sphereRadius;
		Float3 sphereAlbedo;
		Float3 groundAlbedo;

		// Input
		Float3 worldPosition;

		// Output
		Float4 outputColor;
	};
}

