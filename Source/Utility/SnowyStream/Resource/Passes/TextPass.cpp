#include "TextPass.h"

using namespace PaintsNow;
using namespace PaintsNow::NsSnowyStream;

TextPass::TextPass() {}

TObject<IReflect>& TextPass::operator () (IReflect& reflect) {
	BaseClass::operator () (reflect);

	if (reflect.IsReflectProperty()) {
		ReflectProperty(textTransform)[IShader::MetaShader(IRender::Resource::ShaderDescription::VERTEX)];
		ReflectProperty(textShading)[IShader::MetaShader(IRender::Resource::ShaderDescription::FRAGMENT)];
	}

	return *this;
}
