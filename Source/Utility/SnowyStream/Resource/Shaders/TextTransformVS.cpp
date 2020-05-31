#define USE_SWIZZLE
#include "TextTransformVS.h"
#include "../../../../General/Template/TShaderMacro.h"

using namespace PaintsNow;
using namespace PaintsNow::NsSnowyStream;
using namespace PaintsNow::ShaderMacro;

TextTransformVS::TextTransformVS() {
	positionBuffer.description.usage = IRender::Resource::BufferDescription::VERTEX;
	globalBuffer.description.usage = IRender::Resource::BufferDescription::UNIFORM;
}

String TextTransformVS::GetShaderText() {
	return UnifyShaderCode(
		rasterPosition = mult_vec(worldMatrix, float4(unitTexCoord.x, unitTexCoord.y, 0, 1));
		rasterCoord = unitTexCoord.zw;
	);
}

TObject<IReflect>& TextTransformVS::operator () (IReflect& reflect) {
	BaseClass::operator () (reflect);

	if (reflect.IsReflectProperty()) {
		ReflectProperty(positionBuffer);
		ReflectProperty(globalBuffer);

		ReflectProperty(worldMatrix)[globalBuffer][BindInput(BindInput::TRANSFORM_WORLD)];
		ReflectProperty(unitTexCoord)[positionBuffer][BindInput(BindInput::POSITION)];

		ReflectProperty(rasterCoord)[BindOutput(BindOutput::TEXCOORD)];
		ReflectProperty(rasterPosition)[BindOutput(BindOutput::HPOSITION)];
	}

	return *this;
}
