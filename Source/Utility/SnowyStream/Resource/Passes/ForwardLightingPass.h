// ForwardLighting.h
// Standard Physical Based Shader
// By PaintDream (paintdream@paintdream.com)
//

#pragma once
#include "../../../../General/Misc/PassBase.h"
#include "../Shaders/StandardTransformVS.h"
#include "../Shaders/StandardParameterFS.h"
#include "../Shaders/StandardLightingFS.h"

namespace PaintsNow {
	// standard pbr deferred shading Pass using ggx prdf
	class ForwardLightingPass : public TReflected<ForwardLightingPass, PassBase> {
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
