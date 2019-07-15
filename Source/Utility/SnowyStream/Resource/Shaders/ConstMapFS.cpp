#define USE_SWIZZLE
#include "ConstMapFS.h"
#include "../../../../General/Template/TShaderMacro.h"

using namespace PaintsNow;
using namespace PaintsNow::NsSnowyStream;
using namespace PaintsNow::ShaderMacro;

ConstMapFS::ConstMapFS() {
}

String ConstMapFS::GetShaderText() {
	return UnifyShaderCode(
		target = tintColor;
	);
}

TObject<IReflect>& ConstMapFS::operator () (IReflect& reflect) {
	BaseClass::operator () (reflect);

	if (reflect.IsReflectProperty()) {
		ReflectProperty(tintColor)[BindInput(BindInput::GENERAL)];
		ReflectProperty(target)[BindOutput(BindOutput::COLOR)];
	}

	return *this;
}