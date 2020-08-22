#include "ShadowMaskPass.h"

using namespace PaintsNow;

ShadowMaskPass::ShadowMaskPass() {
}

TObject<IReflect>& ShadowMaskPass::operator () (IReflect& reflect) {
	BaseClass::operator () (reflect);
	if (reflect.IsReflectProperty()) {
		ReflectProperty(transform)[IShader::MetaShader(IRender::Resource::ShaderDescription::VERTEX)];
		ReflectProperty(mask)[IShader::MetaShader(IRender::Resource::ShaderDescription::FRAGMENT)];
	}

	return *this;
}