#include "BloomPass.h"

using namespace PaintsNow;
using namespace PaintsNow::NsSnowyStream;

BloomPass::BloomPass() {}

TObject<IReflect>& BloomPass::operator () (IReflect& reflect) {
	BaseClass::operator () (reflect);
	if (reflect.IsReflectProperty()) {
		ReflectProperty(screenTransform)[IShader::MetaShader(IRender::Resource::ShaderDescription::VERTEX)];
		ReflectProperty(screenBloom)[IShader::MetaShader(IRender::Resource::ShaderDescription::FRAGMENT)];
	}

	return *this;
}