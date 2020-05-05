// StandardLightingFS.h
// By PaintDream (paintdream@paintdream.com)
// 2018-4-13
//

#ifndef __STANDARDLIGHTING_FS_H
#define __STANDARDLIGHTING_FS_H

#include "../../../../General/Interface/IShader.h"

namespace PaintsNow {
	namespace NsSnowyStream {
		class StandardLightingFS : public TReflected<StandardLightingFS, IShader> {
		public:
			StandardLightingFS();
			virtual TObject<IReflect>& operator () (IReflect& reflect) override;
			virtual String GetShaderText() override;

			BindTexture lightTexture;
			BindTexture specTexture;
			BindBuffer lightBuffer;
			BindBuffer paramBuffer;

			Float3 viewPosition;
			Float3 viewNormal;
			Float3 baseColor;
			float metallic;
			float roughness;
			float occlusion;
			float shadow;

			enum { MAX_LIGHT_COUNT = 64 };
			std::vector<Float4> lightInfos; // position & color
			uint32_t lightCount;
			MatrixFloat4x4 invWorldNormalMatrix;
			float cubeLevelInv;
			Float2 rasterCoord; // imported

			Float4 mainColor;
		};
	}
}


#endif // __STANDARDLIGHTING_FS_H
