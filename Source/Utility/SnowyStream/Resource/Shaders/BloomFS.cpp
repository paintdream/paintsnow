#define USE_SWIZZLE
#include "BloomFS.h"
#include "../../../../General/Template/TShaderMacro.h"

using namespace PaintsNow;

BloomFS::BloomFS() {
	screenTexture.description.state.type = IRender::Resource::TextureDescription::TEXTURE_2D;
	uniformBloomBuffer.description.usage = IRender::Resource::BufferDescription::UNIFORM;
}

String BloomFS::GetShaderText() {
	return UnifyShaderCode(
		float4 a = texture(screenTexture, rasterCoord + float2(0.5, 1.5) * invScreenSize);
		float4 b = texture(screenTexture, rasterCoord + float2(-1.5, 0.5) * invScreenSize);
		float4 c = texture(screenTexture, rasterCoord + float2(-0.5, -1.5) * invScreenSize);
		float4 d = texture(screenTexture, rasterCoord + float2(1.5, -0.5) * invScreenSize);
		outputColor = (a + b + c + d) * float(0.25);
	);
}

TObject<IReflect>& BloomFS::operator () (IReflect& reflect) {
	BaseClass::operator () (reflect);

	if (reflect.IsReflectProperty()) {
		ReflectProperty(screenTexture);
		ReflectProperty(uniformBloomBuffer);

		ReflectProperty(invScreenSize)[uniformBloomBuffer][BindInput(BindInput::GENERAL)];
		ReflectProperty(rasterCoord)[BindInput(BindInput::TEXCOORD)];
		ReflectProperty(outputColor)[BindOutput(BindOutput::COLOR)];
	}

	return *this;
}
