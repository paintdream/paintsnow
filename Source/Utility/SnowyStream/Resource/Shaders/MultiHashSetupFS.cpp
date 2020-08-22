#define USE_SWIZZLE
#include "MultiHashSetupFS.h"
#include "../../../../General/Template/TShaderMacro.h"

using namespace PaintsNow;

MultiHashSetupFS::MultiHashSetupFS() {}

TObject<IReflect>& MultiHashSetupFS::operator () (IReflect& reflect) {
	BaseClass::operator () (reflect);

	if (reflect.IsReflectProperty()) {
		ReflectProperty(noiseTexture);

		ReflectProperty(rasterCoord)[BindInput(BindInput::TEXCOORD)];
		ReflectProperty(tintColor)[BindOutput(BindOutput::LOCAL)];
	}

	return *this;
}

String MultiHashSetupFS::GetShaderText() {
	return UnifyShaderCode(
		float noise = texture(noiseTexture, rasterCoord.xy * tintColor.z + tintColor.xy).x;
		clip(noise - tintColor.w);
	);
}