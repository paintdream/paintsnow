#define USE_SWIZZLE
#include "MultiHashSetupFS.h"
#include "../../../../General/Template/TShaderMacro.h"

using namespace PaintsNow;

MultiHashSetupFS::MultiHashSetupFS() {}

TObject<IReflect>& MultiHashSetupFS::operator () (IReflect& reflect) {
	BaseClass::operator () (reflect);

	if (reflect.IsReflectProperty()) {
		ReflectProperty(noiseTexture);

		ReflectProperty(rasterCoord)[BindInput(BindInput::RASTERCOORD)];
		ReflectProperty(tintColor)[BindInput(BindInput::LOCAL)];
	}

	return *this;
}

String MultiHashSetupFS::GetShaderText() {
	return UnifyShaderCode(
		float noise = texture(noiseTexture, rasterCoord.xy * tintColor.xy + tintColor.zw).x;
		clip(noise - 0.5);
	);
}