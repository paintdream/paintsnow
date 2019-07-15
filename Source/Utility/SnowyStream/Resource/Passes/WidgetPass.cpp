#include "WidgetPass.h"

using namespace PaintsNow;
using namespace PaintsNow::NsSnowyStream;

WidgetPass::WidgetPass() {
}

TObject<IReflect>& WidgetPass::operator () (IReflect& reflect) {
	BaseClass::operator () (reflect);
	if (reflect.IsReflectProperty()) {
		ReflectProperty(widgetTransform)[IShader::MetaShader(IRender::Resource::ShaderDescription::VERTEX)];
		ReflectProperty(widgetShading)[IShader::MetaShader(IRender::Resource::ShaderDescription::FRAGMENT)];
	}

	return *this;
}