#include "DepthBoundingSetupPass.h"

using namespace PaintsNow;
using namespace PaintsNow::NsSnowyStream;

DepthBoundingSetupPass::DepthBoundingSetupPass() {
}

TObject<IReflect>& DepthBoundingSetupPass::operator () (IReflect& reflect) {
	BaseClass::operator () (reflect);
	if (reflect.IsReflectProperty()) {
		ReflectProperty(transform)[IShader::MetaShader(IRender::Resource::ShaderDescription::VERTEX)];
		ReflectProperty(minmax)[IShader::MetaShader(IRender::Resource::ShaderDescription::FRAGMENT)];
	}

	return *this;
}