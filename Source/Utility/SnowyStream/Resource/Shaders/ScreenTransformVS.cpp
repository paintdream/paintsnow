#define USE_SWIZZLE
#include "ScreenTransformVS.h"
#include "../../../../General/Template/TShaderMacro.h"

using namespace PaintsNow;

ScreenTransformVS::ScreenTransformVS() : enableVertexTransform(false) {
	vertexBuffer.description.usage = IRender::Resource::BufferDescription::VERTEX;
	transformBuffer.description.usage = IRender::Resource::BufferDescription::UNIFORM;
}

String ScreenTransformVS::GetShaderText() {
	return UnifyShaderCode(
		if (enableVertexTransform) {
			position.xyz = vertexPosition.xyz;
			position.w = 1;
			position = mult_vec(worldTransform, position);
			// no near/far clip
			// position.z = lerp(max(position.z, position.w), min(position.z, position.w), step(0, position.z));
			position.xyz /= position.w;
			position.w = 1;
			rasterCoord.xy = position.xy / position.w * float2(0.5, 0.5) + float2(0.5, 0.5);
		} else {
			position.xyz = vertexPosition.xyz;
			rasterCoord.xy = position.xy * float2(0.5, 0.5) + float2(0.5, 0.5);
			position.w = 1;
		}
	);
}

TObject<IReflect>& ScreenTransformVS::operator () (IReflect& reflect) {
	BaseClass::operator () (reflect);

	if (reflect.IsReflectProperty()) {
		ReflectProperty(enableVertexTransform)[BindConst<bool>()];

		ReflectProperty(vertexBuffer);
		ReflectProperty(transformBuffer)[BindOption(enableVertexTransform)];

		ReflectProperty(vertexPosition)[vertexBuffer][BindInput(BindInput::POSITION)];
		ReflectProperty(worldTransform)[BindOption(enableVertexTransform)][transformBuffer][BindInput(BindInput::TRANSFORM_WORLD)];
		ReflectProperty(position)[BindOutput(BindOutput::HPOSITION)];
		ReflectProperty(rasterCoord)[BindOutput(BindOutput::TEXCOORD)];
	}

	return *this;
}