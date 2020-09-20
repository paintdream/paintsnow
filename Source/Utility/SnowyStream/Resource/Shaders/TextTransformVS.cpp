#define USE_SWIZZLE
#include "TextTransformVS.h"
#include "../../../../General/Template/TShaderMacro.h"

using namespace PaintsNow;

TextTransformVS::TextTransformVS() {
	positionBuffer.description.usage = IRender::Resource::BufferDescription::VERTEX;
	instanceBuffer.description.usage = IRender::Resource::BufferDescription::INSTANCED;
	texCoordRectBuffer.description.usage = IRender::Resource::BufferDescription::INSTANCED;
}

String TextTransformVS::GetShaderText() {
	return UnifyShaderCode(
		rasterPosition = mult_vec(worldMatrix, float4(unitTexCoord.x, unitTexCoord.y, 0, 1));
		texCoord = lerp(texCoordRect.xy, texCoordRect.zw, unitTexCoord.xy * float(0.5) + float2(0.5, 0.5));
	);
}

TObject<IReflect>& TextTransformVS::operator () (IReflect& reflect) {
	BaseClass::operator () (reflect);

	if (reflect.IsReflectProperty()) {
		ReflectProperty(instanceBuffer);
		ReflectProperty(texCoordRectBuffer);
		ReflectProperty(positionBuffer);

		ReflectProperty(worldMatrix)[instanceBuffer][BindInput(BindInput::TRANSFORM_WORLD)];
		ReflectProperty(texCoordRect)[texCoordRectBuffer][BindInput(BindInput::GENERAL)];
		ReflectProperty(unitTexCoord)[positionBuffer][BindInput(BindInput::POSITION)];

		ReflectProperty(texCoord)[BindOutput(BindOutput::TEXCOORD)];
		ReflectProperty(rasterPosition)[BindOutput(BindOutput::HPOSITION)];
	}

	return *this;
}
