// CustomMaterialPass.h
// Standard Physical Based Shader
// By PaintDream (paintdream@paintdream.com)
//

#pragma once
#include "../../../../General/Misc/PassBase.h"
#include "../Shaders/StandardTransformVS.h"
#include "../Shaders/CustomMaterialParameterFS.h"
#include "../Shaders/DeferredCompactFS.h"
#include "CustomizeShader.h"

namespace PaintsNow {
	// standard pbr deferred shading Pass using ggx brdf
	class CustomMaterialPass : public TReflected<CustomMaterialPass, PassBase>, public ICustomizeShader {
	public:
		CustomMaterialPass();
		virtual TObject<IReflect>& operator () (IReflect& reflect) override;
		void SetInput(const String& stage, const String& type, const String& name, const std::vector<std::pair<String, String> >& config);
		void SetCode(const String& stage, const String& code, const std::vector<std::pair<String, String> >& config);
		void SetComplete();

	protected:
		// Vertex shaders
		StandardTransformVS screenTransform;
		// Fragment shaders
		CustomMaterialParameterFS shaderParameter;
		DeferredCompactEncodeFS shaderCompactEncode;
	};
}
