#include "LightBufferPass.h"

using namespace PaintsNow;
using namespace PaintsNow::NsSnowyStream;

LightBufferPass::LightBufferPass() {
}

TObject<IReflect>& LightBufferPass::operator () (IReflect& reflect) {
	BaseClass::operator () (reflect);
	if (reflect.IsReflectProperty()) {
		ReflectProperty(transform)[IShader::MetaShader(IRender::Resource::ShaderDescription::VERTEX)];
		ReflectProperty(encoder)[IShader::MetaShader(IRender::Resource::ShaderDescription::FRAGMENT)];
	}

	return *this;
}