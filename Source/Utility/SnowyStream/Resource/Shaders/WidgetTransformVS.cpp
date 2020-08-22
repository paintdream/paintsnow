#define USE_SWIZZLE
#include "WidgetTransformVS.h"
#include "../../../../General/Template/TShaderMacro.h"

using namespace PaintsNow;

WidgetTransformVS::WidgetTransformVS() {
	vertexBuffer.description.usage = IRender::Resource::BufferDescription::VERTEX;
	instanceBuffer.description.usage = IRender::Resource::BufferDescription::INSTANCED;
}

String WidgetTransformVS::GetShaderText() {
	return UnifyShaderCode(
		position.xyz = unitPositionTexCoord;
		position.w = 1;
		position = mult_vec(worldMatrix, position);

		// copy outputs
		texCoord = unitPositionTexCoord.xyxy;

		float4 scale = mult_vec(worldMatrix, float4(1, 1, 0, 0));
		scale.zw = subTexMark.xy + float2(1.0, 1.0) - subTexMark.zw; // in ratio
		scale.xy = (float2(1.0, 1.0) - scale.zw) / scale.xy;
		texCoordRect = mainCoordRect;
		texCoordScale.xy = (float2(1.0, 1.0) - scale.xy) / scale.zw;
		texCoordScale.zw = scale.xy;
		texCoordMark.xy = subTexMark.xy * scale.xy;
		texCoordMark.zw = float2(1.0, 1.0) - (float2(1.0, 1.0) - subTexMark.zw) * scale.xy;
	);
}

TObject<IReflect>& WidgetTransformVS::operator () (IReflect& reflect) {
	BaseClass::operator () (reflect);

	if (reflect.IsReflectProperty()) {
		ReflectProperty(vertexBuffer);
		ReflectProperty(instanceBuffer);

		ReflectProperty(unitPositionTexCoord)[vertexBuffer][BindInput(BindInput::POSITION)];
		ReflectProperty(worldMatrix)[instanceBuffer][BindInput(BindInput::TRANSFORM_WORLD)];
		ReflectProperty(subTexMark)[instanceBuffer][BindInput(BindInput::TEXCOORD)];
		ReflectProperty(mainCoordRect)[instanceBuffer][BindInput(BindInput::TEXCOORD + 1)];

		ReflectProperty(position)[BindOutput(BindOutput::HPOSITION)];
		ReflectProperty(texCoord)[BindOutput(BindOutput::TEXCOORD)];
		ReflectProperty(texCoordRect)[BindOutput(BindOutput::TEXCOORD + 1)];
		ReflectProperty(texCoordMark)[BindOutput(BindOutput::TEXCOORD + 2)];
		ReflectProperty(texCoordScale)[BindOutput(BindOutput::TEXCOORD + 3)];
	}

	return *this;
}