#define USE_SWIZZLE
#include "SkyTransformVS.h"
#include "../../../../General/Template/TShaderMacro.h"

using namespace PaintsNow;

SkyTransformVS::SkyTransformVS() {}

String SkyTransformVS::GetShaderText() {
	return UnifyShaderCode(
		int a = 0;
	);
}

TObject<IReflect>& SkyTransformVS::operator () (IReflect& reflect) {
	BaseClass::operator () (reflect);

	if (reflect.IsReflectProperty()) {
	}

	return *this;
}
