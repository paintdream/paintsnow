// DeferredCompact.h
// By PaintDream (paintdream@paintdream.com)
// 2018-4-13
//

#pragma once
#include "../../../../General/Interface/IShader.h"

namespace PaintsNow {
	class TileBasedLightCS : public TReflected<TileBasedLightCS, IShader> {
	public:
		TileBasedLightCS();
		TObject<IReflect>& operator () (IReflect& reflect) override;
		String GetShaderText() override;

		BindTexture depthTexture;
		BindBuffer lightBuffer;
		enum { MAX_LIGHT_COUNT = 120 };

		// uniforms
		MatrixFloat4x4 inverseProjectionMatrix;
		Float2 invScreenSize;
		float lightCount;
		float reserved;
		std::vector<Float4> lightInfos;

		// outputs
		IShader::BindBuffer encodeBuffer;
		std::vector<uint32_t> encodeBufferData;
	};
}


