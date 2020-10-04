// CustomMaterialPass.h
// Standard Physical Based Shader
// By PaintDream (paintdream@paintdream.com)
//

#pragma once
#include "../../../../General/Misc/PassBase.h"
#include "../Shaders/CustomMaterialTransformVS.h"
#include "../Shaders/CustomMaterialParameterFS.h"
#include "../Shaders/DeferredCompactFS.h"

namespace PaintsNow {
	// standard pbr deferred shading Pass using ggx brdf
	class CustomMaterialPass : public TReflected<CustomMaterialPass, PassBase> {
	public:
		CustomMaterialPass();
		TObject<IReflect>& operator () (IReflect& reflect) override;
		void SetInput(const String& stage, const String& type, const String& name, const String& value, const std::vector<std::pair<String, String> >& config);
		void SetCode(const String& stage, const String& code, const std::vector<std::pair<String, String> >& config);
		void SetComplete();

	protected:
		// Vertex shaders
		CustomMaterialTransformVS shaderTransform;
		// Fragment shaders
		CustomMaterialParameterFS shaderParameter;
		DeferredCompactEncodeFS shaderCompactEncode;
	};
}
