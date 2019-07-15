#define USE_SWIZZLE
#include "ShadowMapVS.h"
#include "../../../../General/Template/TShaderMacro.h"

using namespace PaintsNow;
using namespace PaintsNow::NsSnowyStream;

ShadowMapVS::ShadowMapVS() {
	/*
	shaderTextBody = UnifyShaderCode(
		float v = 0;
	);*/
}

TObject<IReflect>& ShadowMapVS::operator () (IReflect& reflect) {
	BaseClass::operator () (reflect);

	if (reflect.IsReflectProperty()) {
	}

	return *this;
}
