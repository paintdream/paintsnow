// CustomMaterialParameterFS.h
// By PaintDream (paintdream@paintdream.com)
// 2018-4-13
//

#pragma once
#include "../../../../General/Interface/IShader.h"
#include "../../../../General/Interface/IAsset.h"
#include "../../../../Core/System/Tiny.h"

namespace PaintsNow {
	class CustomShaderDescription : public TReflected<CustomShaderDescription, SharedTiny> {
	public:
		CustomShaderDescription();
		TObject<IReflect>& operator () (IReflect& reflect) override;

		void SetCode(const String& text);
		void SetInput(const String& category, const String& type, const String& name, const String& value, const std::vector<std::pair<String, String> >& config);
		void SetComplete();

		void ReflectVertexTemplate(IReflect& reflect);
		void ReflectUniformTemplate(IReflect& reflect);
		void ReflectInputTemplate(IReflect& reflect);
		void ReflectOutputTemplate(IReflect& reflect);

		IAsset::Material vertexTemplate;
		IAsset::Material uniformTemplate;
		IAsset::Material inputTemplate;
		IAsset::Material outputTemplate;

		std::vector<IShader::BindTexture> uniformTextureBindings;
		std::vector<IShader::BindBuffer> vertexBufferBindings;
		Bytes uniformBlock;
		String code;
		IShader::BindBuffer uniformBuffer;
	};
}


