#include "CustomMaterialPass.h"

using namespace PaintsNow;

CustomMaterialPass::CustomMaterialPass() {}

TObject<IReflect>& CustomMaterialPass::operator () (IReflect& reflect) {
	BaseClass::operator () (reflect);

	if (reflect.IsReflectProperty()) {
		ReflectProperty(shaderTransform)[IShader::MetaShader(IRender::Resource::ShaderDescription::VERTEX)];
		ReflectProperty(shaderParameter)[IShader::MetaShader(IRender::Resource::ShaderDescription::FRAGMENT)];
		ReflectProperty(shaderCompactEncode)[IShader::MetaShader(IRender::Resource::ShaderDescription::FRAGMENT)];
	}

	return *this;
}

void CustomMaterialPass::SetInput(const String& stage, const String& type, const String& name, const String& value, const String& binding, const std::vector<std::pair<String, String> >& config) {
	if (stage == "VertexInput") {
		shaderTransform.description->SetInput("Input", type, name, value, binding, config);
	} else if (stage == "VertexOptions") {
		shaderTransform.description->SetInput("Option", type, name, value, binding, config);
	} else if (stage == "VertexUniform") {
		shaderTransform.description->SetInput("Uniform", type, name, value, binding, config);
	} else if (stage == "VertexVarying") {
		shaderTransform.description->SetInput("Output", type, name, value, binding, config);
		shaderParameter.description->SetInput("Input", type, name, value, binding, config);
	} else if (stage == "FragmentOptions") {
		shaderParameter.description->SetInput("Option", type, name, value, binding, config);
	} else if (stage == "FragmentUniform") {
		shaderParameter.description->SetInput("Uniform", type, name, value, binding, config);
	}
}

void CustomMaterialPass::SetCode(const String& stage, const String& code, const std::vector<std::pair<String, String> >& config) {
	if (stage == "Vertex") {
		shaderTransform.description->SetCode(code);
	} else if (stage == "Fragment") {
		shaderParameter.description->SetCode(code);
	}
}

void CustomMaterialPass::SetComplete() {
	uniformData.Clear();
	optionData.Clear();

	std::vector<String> defTexturePaths;
	shaderTransform.description->SetComplete(uniformData, optionData);
	shaderParameter.description->SetComplete(uniformData, optionData);
}