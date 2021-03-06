#define USE_SWIZZLE
#include "WidgetShadingFS.h"
#include "../../../../General/Template/TShaderMacro.h"

using namespace PaintsNow;

WidgetShadingFS::WidgetShadingFS() {
	mainTexture.description.state.type = IRender::Resource::TextureDescription::TEXTURE_2D;
}

String WidgetShadingFS::GetShaderText() {
	return UnifyShaderCode(
		float4 offset = max(texCoord.xyxy - texCoordMark.xyzw, float4(0.0, 0.0, 0.0, 0.0));
		float2 minCoord = min(texCoord.xy, texCoordMark.xy);
		float2 newCoord = minCoord * texCoordScale.xy;
		newCoord.xy = newCoord.xy + texCoordScale.zw * (offset.xy - offset.zw);
		newCoord.xy = newCoord.xy + texCoordScale.xy * offset.zw;
		float2 coord = lerp(texCoordRect.xy, texCoordRect.zw, newCoord.xy);
		target = texture(mainTexture, coord.xy);
	);
}

TObject<IReflect>& WidgetShadingFS::operator () (IReflect& reflect) {
	BaseClass::operator () (reflect);

	if (reflect.IsReflectProperty()) {
		ReflectProperty(mainTexture)[BindInput(BindInput::MAINTEXTURE)];

		ReflectProperty(texCoord)[BindInput(BindInput::TEXCOORD)];
		ReflectProperty(texCoordRect)[BindInput(BindInput::TEXCOORD)];
		ReflectProperty(texCoordScale)[BindInput(BindInput::TEXCOORD)];
		ReflectProperty(texCoordMark)[BindInput(BindInput::TEXCOORD)];
		ReflectProperty(target)[BindOutput(BindOutput::COLOR)];
	}

	return *this;
}