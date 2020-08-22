#include "DeferredLightingPass.h"

using namespace PaintsNow;

DeferredLightingPass::DeferredLightingPass() {}

TObject<IReflect>& DeferredLightingPass::operator () (IReflect& reflect) {
	BaseClass::operator () (reflect);

	if (reflect.IsReflectProperty()) {
		ReflectProperty(screenTransform)[IShader::MetaShader(IRender::Resource::ShaderDescription::VERTEX)];
		ReflectProperty(deferredCompactDecode)[IShader::MetaShader(IRender::Resource::ShaderDescription::FRAGMENT)];
		ReflectProperty(standardLighting)[IShader::MetaShader(IRender::Resource::ShaderDescription::FRAGMENT)];
	}

	return *this;
}