// StandardPass.h
// Standard Physically Based Shader
// By PaintDream (paintdream@paintdream.com)
//

#ifndef __STANDARD_PASS_H__
#define __STANDARD_PASS_H__

#include "../../../../General/Misc/PassBase.h"
#include "../Shaders/StandardTransformVS.h"
#include "../Shaders/StandardParameterFS.h"
#include "../Shaders/DeferredCompactFS.h"

namespace PaintsNow {
	namespace NsSnowyStream {
		// standard pbr deferred shading Pass using ggx prdf
		class StandardPass : public TReflected<StandardPass, PassBase> {
		public:
			StandardPass();
			virtual TObject<IReflect>& operator () (IReflect& reflect) override;

		protected:
			// Vertex shaders
			StandardTransformVS screenTransform;
			// Fragment shaders
			StandardParameterFS shaderParameter;
			DeferredCompactEncodeFS shaderCompactEncode;
		};
	}
}


#endif // __STANDARD_PASS_H__
