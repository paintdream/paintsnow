// DeferredCompact.h
// By PaintDream (paintdream@paintdream.com)
// 2018-4-13
//

#ifndef __DEFERREDCOMPACT_FS_H
#define __DEFERREDCOMPACT_FS_H

#include "../../../../General/Interface/IShader.h"

namespace PaintsNow {
	namespace NsSnowyStream {
		class DeferredCompactEncodeFS : public TReflected<DeferredCompactEncodeFS, IShader> {
		public:
			DeferredCompactEncodeFS();
			virtual TObject<IReflect>& operator () (IReflect& reflect) override;
			virtual String GetShaderText() override;

		protected:
			// imports
			Float4 outputColor;
			Float3 outputNormal;
			float occlusion;
			float metallic;
			float roughness;

			// outputs
			Float4 encodeBaseColorOcclusion;
			Float4 encodeNormalRoughnessMetallic;
		};

		class DeferredCompactDecodeFS : public TReflected<DeferredCompactDecodeFS, IShader> {
		public:
			DeferredCompactDecodeFS();
			virtual TObject<IReflect>& operator () (IReflect& reflect) override;
			virtual String GetShaderText() override;

			// input
			IShader::BindTexture BaseColorOcclusionTexture;
			IShader::BindTexture NormalRoughnessMetallicTexture;
			IShader::BindTexture DepthTexture;
			IShader::BindBuffer uniformProjectionBuffer;

			MatrixFloat4x4 inverseProjectionMatrix;
			Float2 rasterCoord;

			// outputs
			Float3 viewPosition;
			Float3 viewNormal;
			Float3 baseColor;
			float occlusion;
			float metallic;
			float roughness;
			float depth;
		};
	}
}


#endif // __DEFERREDCOMPACT_FS_H
