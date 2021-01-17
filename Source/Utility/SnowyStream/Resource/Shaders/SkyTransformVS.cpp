#define USE_SWIZZLE
#include "SkyTransformVS.h"
#include "../../../../General/Template/TShaderMacro.h"

using namespace PaintsNow;

SkyTransformVS::SkyTransformVS() {
	globalBuffer.description.usage = IRender::Resource::BufferDescription::UNIFORM;
	vertexPositionBuffer.description.usage = IRender::Resource::BufferDescription::VERTEX;
}

String SkyTransformVS::GetShaderText() {
	return UnifyShaderCode(
		float4 position = float4(0, 0, 0, 1);
		position.xyz = vertexPosition;
		position = mult_vec(worldMatrix, position);
		rasterPosition = mult_vec(viewProjectionMatrix, position);
		worldPosition = position.xyz;
	);
}

TObject<IReflect>& SkyTransformVS::operator () (IReflect& reflect) {
	BaseClass::operator () (reflect);

	if (reflect.IsReflectProperty()) {
		ReflectProperty(globalBuffer);
		ReflectProperty(vertexPositionBuffer);

		ReflectProperty(worldMatrix)[globalBuffer][BindInput(BindInput::TRANSFORM_WORLD)];
		ReflectProperty(viewProjectionMatrix)[globalBuffer][BindInput(BindInput::TRANSFORM_VIEWPROJECTION)];

		ReflectProperty(vertexPosition)[vertexPositionBuffer][BindInput(BindInput::POSITION)];
		ReflectProperty(rasterPosition)[BindOutput(BindOutput::HPOSITION)];
		ReflectProperty(worldPosition)[BindOutput(BindOutput::TEXCOORD)];
	}

	return *this;
}
