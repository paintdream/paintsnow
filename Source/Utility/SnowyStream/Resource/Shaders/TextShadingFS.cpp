#define USE_SWIZZLE
#include "TextShadingFS.h"
#include "../../../../General/Template/TShaderMacro.h"

using namespace PaintsNow;
using namespace PaintsNow::NsSnowyStream;
using namespace PaintsNow::ShaderMacro;

TextShadingFS::TextShadingFS() {
	mainTexture.description.state.type = IRender::Resource::TextureDescription::TEXTURE_2D;
}

String TextShadingFS::GetShaderText() {
	return UnifyShaderCode(
		target = texture(mainTexture, rasterCoord.xy);
	);
}

TObject<IReflect>& TextShadingFS::operator () (IReflect& reflect) {
	BaseClass::operator () (reflect);

	if (reflect.IsReflectProperty()) {
		ReflectProperty(mainTexture)[BindInput(BindInput::MAINTEXTURE)];

		ReflectProperty(rasterCoord)[BindInput(BindInput::TEXCOORD)];
		ReflectProperty(target)[BindOutput(BindOutput::COLOR)];
	}

	return *this;
}