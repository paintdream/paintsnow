// DeferredLighting.h
// Standard Physical Based Shader
// By PaintDream (paintdream@paintdream.com)
//

#ifndef __DEFERREDLIGHTING_PASS_H__
#define __DEFERREDLIGHTING_PASS_H__

#include "../../../../General/Misc/PassBase.h"
#include "../Shaders/ScreenTransformVS.h"
#include "../Shaders/DeferredCompactFS.h"
#include "../Shaders/StandardLightingFS.h"

namespace PaintsNow {
	namespace NsSnowyStream {
		// standard pbr deferred shading Pass using ggx brdf
		class DeferredLightingPass : public TReflected<DeferredLightingPass, PassBase> {
		public:
			DeferredLightingPass();
			virtual TObject<IReflect>& operator () (IReflect& reflect) override;

			// Vertex shaders
			ScreenTransformVS screenTransform;
			// Fragment shaders
			DeferredCompactDecodeFS deferredCompactDecode;
			StandardLightingFS standardLighting;
		};
	}
}


#endif // __DEFERREDLIGHTING_PASS_H__
