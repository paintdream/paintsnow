#define USE_SWIZZLE
#include "ScreenSpaceFilterFS.h"
#include "../../../../General/Template/TShaderMacro.h"

using namespace PaintsNow;

ScreenSpaceFilterFS::ScreenSpaceFilterFS() {
}

String ScreenSpaceFilterFS::GetShaderText() {
	return UnifyShaderCode(
		float4 color = float4(0, 0, 0, 0);
	);
}

TObject<IReflect>& ScreenSpaceFilterFS::operator () (IReflect& reflect) {
	BaseClass::operator () (reflect);

	if (reflect.IsReflectProperty()) {
	}

	return *this;
}
