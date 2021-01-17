#define USE_SWIZZLE
#include "SkyShadingFS.h"
#include "../../../../General/Template/TShaderMacro.h"

using namespace PaintsNow;

SkyShadingFS::SkyShadingFS() {
}

String SkyShadingFS::GetShaderText() {
	return UnifyShaderCode(
		int a = 0;
	);
}

TObject<IReflect>& SkyShadingFS::operator () (IReflect& reflect) {
	BaseClass::operator () (reflect);

	if (reflect.IsReflectProperty()) {
	}

	return *this;
}