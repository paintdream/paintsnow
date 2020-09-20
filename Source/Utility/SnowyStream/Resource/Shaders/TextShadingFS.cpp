#define USE_SWIZZLE
#include "TextShadingFS.h"
#include "../../../../General/Template/TShaderMacro.h"

using namespace PaintsNow;

TextShadingFS::TextShadingFS() {
	mainTexture.description.state.type = IRender::Resource::TextureDescription::TEXTURE_2D;
}

String TextShadingFS::GetShaderText() {
	return UnifyShaderCode(
		target = texture(mainTexture, texCoord.xy);
	);
}

TObject<IReflect>& TextShadingFS::operator () (IReflect& reflect) {
	BaseClass::operator () (reflect);

	if (reflect.IsReflectProperty()) {
		ReflectProperty(mainTexture)[BindInput(BindInput::MAINTEXTURE)];

		ReflectProperty(texCoord)[BindInput(BindInput::TEXCOORD)];
		ReflectProperty(target)[BindOutput(BindOutput::COLOR)];
	}

	return *this;
}