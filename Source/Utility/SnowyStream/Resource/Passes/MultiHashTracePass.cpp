#include "MultiHashTracePass.h"

using namespace PaintsNow;
using namespace PaintsNow::NsSnowyStream;

MultiHashTracePass::MultiHashTracePass() {}

TObject<IReflect>& MultiHashTracePass::operator () (IReflect& reflect) {
	BaseClass::operator () (reflect);

	if (reflect.IsReflectProperty()) {
		ReflectProperty(screenTransform)[IShader::MetaShader(IRender::Resource::ShaderDescription::VERTEX)];
		ReflectProperty(shaderMultiHashTrace)[IShader::MetaShader(IRender::Resource::ShaderDescription::FRAGMENT)];
	}

	return *this;
}