#include "CustomMaterialPass.h"

using namespace PaintsNow;

CustomMaterialPass::CustomMaterialPass() {}

TObject<IReflect>& CustomMaterialPass::operator () (IReflect& reflect) {
	BaseClass::operator () (reflect);

	if (reflect.IsReflectProperty()) {
		ReflectProperty(screenTransform)[IShader::MetaShader(IRender::Resource::ShaderDescription::VERTEX)];
		ReflectProperty(shaderParameter)[IShader::MetaShader(IRender::Resource::ShaderDescription::FRAGMENT)];
		ReflectProperty(shaderCompactEncode)[IShader::MetaShader(IRender::Resource::ShaderDescription::FRAGMENT)];
	}

	return *this;
}

void CustomMaterialPass::SetInput(const String& stage, const String& type, const String& name, const std::vector<std::pair<String, String> >& config) {
	if (stage == "Material") {
		shaderParameter.SetInput(type, name, config);
	}
}

void CustomMaterialPass::SetCode(const String& stage, const String& code, const std::vector<std::pair<String, String> >& config) {
	if (stage == "Material") {
		shaderParameter.SetCode(code);
	}
}

void CustomMaterialPass::SetComplete() {
	shaderParameter.SetComplete();
}