// BloomPass.h
// BloomFS Shader
// By PaintDream (paintdream@paintdream.com)
//

#ifndef __BLOOM_PASS_H__
#define __BLOOM_PASS_H__

#include "../../../../General/Misc/ZPassBase.h"
#include "../Shaders/ScreenTransformVS.h"
#include "../Shaders/BloomFS.h"
#include "../Shaders/DeferredCompactFS.h"

namespace PaintsNow {
	namespace NsSnowyStream {
		// standard pbr deferred shading Pass using ggx prdf
		class BloomPass : public TReflected<BloomPass, ZPassBase> {
		public:
			BloomPass();
			virtual TObject<IReflect>& operator () (IReflect& reflect) override;

			// Vertex shaders
			ScreenTransformVS screenTransform;
			// Fragment shaders
			BloomFS screenBloom;
		};
	}
}


#endif // __BLOOM_PASS_H__
