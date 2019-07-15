#include "MultiHashLightPass.h"

using namespace PaintsNow;
using namespace PaintsNow::NsSnowyStream;

MultiHashLightPass::MultiHashLightPass() {}

TObject<IReflect>& MultiHashLightPass::operator () (IReflect& reflect) {
	BaseClass::operator () (reflect);

	if (reflect.IsReflectProperty()) {
		ReflectProperty(screenTransform)[IShader::MetaShader(IRender::Resource::ShaderDescription::VERTEX)];
		ReflectProperty(shaderMultiHashLight)[IShader::MetaShader(IRender::Resource::ShaderDescription::FRAGMENT)];
	}

	return *this;
}