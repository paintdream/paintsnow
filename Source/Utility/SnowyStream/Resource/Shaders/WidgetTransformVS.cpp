#define USE_SWIZZLE
#include "WidgetTransformVS.h"
#include "../../../../General/Template/TShaderMacro.h"

using namespace PaintsNow;
using namespace PaintsNow::NsSnowyStream;
using namespace PaintsNow::ShaderMacro;

WidgetTransformVS::WidgetTransformVS() {
	vertexBuffer.description.usage = IRender::Resource::BufferDescription::VERTEX;
	instanceBuffer.description.usage = IRender::Resource::BufferDescription::INSTANCED;
}

String WidgetTransformVS::GetShaderText() {
	return UnifyShaderCode(
		position.xy = lerp(inputPositionRect.xy, inputPositionRect.zw, unitPositionTexCoord.xy);
		position.z = unitPositionTexCoord.z;
		position.w = 1;

		// copy outputs
		texCoord = unitPositionTexCoord.xyxy;
		texCoordRect = inputTexCoordRect;
		texCoordMark = inputTexCoordMark;
		texCoordScale = inputTexCoordScale;
		tintColor = inputTintColor;	
	);
}

TObject<IReflect>& WidgetTransformVS::operator () (IReflect& reflect) {
	BaseClass::operator () (reflect);

	if (reflect.IsReflectProperty()) {
		ReflectProperty(vertexBuffer);
		ReflectProperty(instanceBuffer);

		ReflectProperty(unitPositionTexCoord)[vertexBuffer][BindInput(BindInput::POSITION)];
		ReflectProperty(inputPositionRect)[instanceBuffer][BindInput(BindInput::TEXCOORD)];
		ReflectProperty(inputTexCoordRect)[instanceBuffer][BindInput(BindInput::TEXCOORD + 1)];
		ReflectProperty(inputTexCoordMark)[instanceBuffer][BindInput(BindInput::TEXCOORD + 2)];
		ReflectProperty(inputTexCoordScale)[instanceBuffer][BindInput(BindInput::TEXCOORD + 3)];
		ReflectProperty(inputTintColor)[instanceBuffer][BindInput(BindInput::COLOR)];

		ReflectProperty(position)[BindOutput(BindOutput::HPOSITION)];
		ReflectProperty(texCoord)[BindOutput(BindOutput::TEXCOORD)];
		ReflectProperty(texCoordRect)[BindOutput(BindOutput::TEXCOORD + 1)];
		ReflectProperty(texCoordMark)[BindOutput(BindOutput::TEXCOORD + 2)];
		ReflectProperty(texCoordScale)[BindOutput(BindOutput::TEXCOORD + 3)];
		ReflectProperty(tintColor)[BindOutput(BindOutput::COLOR)];
	}

	return *this;
}