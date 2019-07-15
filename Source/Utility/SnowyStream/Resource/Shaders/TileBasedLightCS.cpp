#define USE_SWIZZLE
#include "TileBasedLightCS.h"
#include "../../../../General/Template/TShaderMacro.h"

using namespace PaintsNow;
using namespace PaintsNow::NsSnowyStream;
using namespace PaintsNow::ShaderMacro;

TileBasedLightCS::TileBasedLightCS() {
}

String TileBasedLightCS::GetShaderText() {
	return UnifyShaderCode(
		float4 todo;
	);
}

TObject<IReflect>& TileBasedLightCS::operator () (IReflect& reflect) {
	BaseClass::operator () (reflect);

	if (reflect.IsReflectProperty()) {
		// inputs
	}

	return *this;
}
