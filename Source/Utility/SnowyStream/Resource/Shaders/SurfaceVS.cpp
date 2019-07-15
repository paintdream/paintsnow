#define USE_SWIZZLE
#include "SurfaceVS.h"
#include "../../../../General/Template/TShaderMacro.h"

using namespace PaintsNow;
using namespace PaintsNow::NsSnowyStream;
using namespace PaintsNow::ShaderMacro;

SurfaceVS::SurfaceVS() {
}

String SurfaceVS::GetShaderText() {
	return UnifyShaderCode(
		float a = 0; // TODO:
	);
}

TObject<IReflect>& SurfaceVS::operator () (IReflect& reflect) {
	BaseClass::operator () (reflect);

	if (reflect.IsReflectProperty()) {
	}

	return *this;
}
