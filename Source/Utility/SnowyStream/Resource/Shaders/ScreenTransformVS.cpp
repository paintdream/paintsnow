#define USE_SWIZZLE
#include "ScreenTransformVS.h"
#include "../../../../General/Template/TShaderMacro.h"

using namespace PaintsNow;
using namespace PaintsNow::NsSnowyStream;
using namespace PaintsNow::ShaderMacro;

ScreenTransformVS::ScreenTransformVS() {
	vertexBuffer.description.usage = IRender::Resource::BufferDescription::VERTEX;
}

String ScreenTransformVS::GetShaderText() {
	return UnifyShaderCode(
		position.xyz = unitPositionTexCoord.xyz;
		rasterCoord.xy = position.xy * float2(0.5, 0.5) + float2(0.5, 0.5);
		position.w = 1;
	);
}

TObject<IReflect>& ScreenTransformVS::operator () (IReflect& reflect) {
	BaseClass::operator () (reflect);

	if (reflect.IsReflectProperty()) {
		ReflectProperty(vertexBuffer);
		ReflectProperty(unitPositionTexCoord)[vertexBuffer][BindInput(BindInput::POSITION)];
		ReflectProperty(position)[BindOutput(BindOutput::HPOSITION)];
		ReflectProperty(rasterCoord)[BindOutput(BindOutput::TEXCOORD)];
	}

	return *this;
}