#define USE_SWIZZLE
#include "ScreenTransformVS.h"
#include "../../../../General/Template/TShaderMacro.h"

using namespace PaintsNow;

ScreenTransformVS::ScreenTransformVS() : enableVertexTransform(false), enableRasterCoord(true) {
	vertexBuffer.description.usage = IRender::Resource::BufferDescription::VERTEX;
	transformBuffer.description.usage = IRender::Resource::BufferDescription::UNIFORM;
}

String ScreenTransformVS::GetShaderText() {
	return UnifyShaderCode(
		if (enableRasterCoord) {
			rasterCoord.xy = vertexPosition.xy * float2(0.5, 0.5) + float2(0.5, 0.5);
		}

		if (enableVertexTransform) {
			position.xyz = vertexPosition.xyz;
			position.w = 1;
			position = mult_vec(worldTransform, position);
			// no far clip
			// position.z = min(position.z, position.w);
		} else {
			position.xyz = vertexPosition.xyz;
			position.w = 1;
		}
	);
}

TObject<IReflect>& ScreenTransformVS::operator () (IReflect& reflect) {
	BaseClass::operator () (reflect);

	if (reflect.IsReflectProperty()) {
		ReflectProperty(enableVertexTransform)[BindConst<bool>()];
		ReflectProperty(enableRasterCoord)[BindConst<bool>()];

		ReflectProperty(vertexBuffer);
		ReflectProperty(transformBuffer)[BindOption(enableVertexTransform)];

		ReflectProperty(vertexPosition)[vertexBuffer][BindInput(BindInput::POSITION)];
		ReflectProperty(worldTransform)[BindOption(enableVertexTransform)][transformBuffer][BindInput(BindInput::TRANSFORM_WORLD)];
		ReflectProperty(position)[BindOutput(BindOutput::HPOSITION)];
		ReflectProperty(rasterCoord)[BindOption(enableRasterCoord)][BindOutput(BindOutput::TEXCOORD)] ;
	}

	return *this;
}