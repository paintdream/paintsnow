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

		void SetCode(const String& text);
		void SetInput(const String& category, const String& type, const String& name, const String& value, const String& binding, const std::vector<std::pair<String, String> >& config);
		void SetComplete(Bytes& extUniformBuffer, Bytes& extOptionBuffer);
		void ReflectExternal(IReflect& reflect, Bytes& extUniformBuffer, Bytes& extOptionBuffer);

		void ReflectVertexTemplate(IReflect& reflect);
		void ReflectOptionTemplate(IReflect& reflect, Bytes& data);
		void ReflectUniformTemplate(IReflect& reflect, Bytes& data);
		void ReflectInputTemplate(IReflect& reflect);
		void ReflectOutputTemplate(IReflect& reflect);

		IShader::BindBuffer uniformBuffer;
		String code;

		IAsset::Material vertexTemplate;
		IAsset::Material uniformTemplate;
		IAsset::Material optionTemplate;
		IAsset::Material inputTemplate;
		IAsset::Material outputTemplate;

		std::vector<IShader::BindTexture> uniformTextureBindings;
		std::vector<IShader::BindBuffer> vertexBufferBindings;
	};
}


