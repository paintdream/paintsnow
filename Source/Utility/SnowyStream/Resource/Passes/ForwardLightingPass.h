// ForwardLighting.h
// Standard Physical Based Shader
// By PaintDream (paintdream@paintdream.com)
//

#ifndef __FORWARDLIGHTING_PASS_H__
#define __FORWARDLIGHTING_PASS_H__

#include "../../../../General/Misc/ZPassBase.h"
#include "../Shaders/StandardTransformVS.h"
#include "../Shaders/StandardParameterFS.h"
#include "../Shaders/StandardLightingFS.h"

namespace PaintsNow {
	namespace NsSnowyStream {
		// standard pbr deferred shading Pass using ggx prdf
		class ForwardLightingPass : public TReflected<ForwardLightingPass, ZPassBase> {
		public:
			ForwardLightingPass();
			virtual TObject<IReflect>& operator () (IReflect& reflect) override;
			// Vertex shaders
			StandardTransformVS standardTransform;
			// Fragment shaders
			StandardParameterFS standardParameter;
			StandardLightingFS standardLighting;
		};
	}
}


#endif // __FORWARDLIGHTING_PASS_H__
