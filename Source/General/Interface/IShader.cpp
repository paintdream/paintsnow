#include "IShader.h"
using namespace PaintsNow;

IShader::MetaShader::MetaShader(IRender::Resource::ShaderDescription::Stage v) : shaderType(v) {}

IShader::MetaShader IShader::MetaShader::operator = (IRender::Resource::ShaderDescription::Stage value) {
	return MetaShader(value);
}

TObject<IReflect>& IShader::MetaShader::operator () (IReflect& reflect) {
	BaseClass::operator () (reflect);
	if (reflect.IsReflectProperty()) {
		ReflectProperty(shaderType);
	}

	return *this;
}

String IShader::GetShaderText() {
	return "";
}

int IShader::WorkGroupSize;
int IShader::NumWorkGroups;
int IShader::LocalInvocationID;
int IShader::WorkGroupID;
int IShader::GlobalInvocationID;
int IShader::LocalInvocationIndex;
