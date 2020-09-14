// DeferredLighting.h
// Standard Physical Based Shader
// By PaintDream (paintdream@paintdream.com)
//

#pragma once
#include "../../../../General/Misc/PassBase.h"
#include "../Shaders/ScreenTransformVS.h"
#include "../Shaders/DeferredCompactFS.h"
#include "../Shaders/StandardLightingFS.h"

namespace PaintsNow {
	// standard pbr deferred shading Pass using ggx brdf
	class DeferredLightingPass : public TReflected<DeferredLightingPass, PassBase> {
	public:
		DeferredLightingPass();
		TObject<IReflect>& operator () (IReflect& reflect) override;

		// Vertex shaders
		ScreenTransformVS screenTransform;
		// Fragment shaders
		DeferredCompactDecodeFS deferredCompactDecode;
		StandardLightingFS standardLighting;
	};
}
