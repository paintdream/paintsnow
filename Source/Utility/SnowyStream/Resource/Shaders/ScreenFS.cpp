#define USE_SWIZZLE
#include "ScreenFS.h"
#include "../../../../General/Template/TShaderMacro.h"

using namespace PaintsNow;
using namespace PaintsNow::NsSnowyStream;
using namespace PaintsNow::ShaderMacro;

ScreenFS::ScreenFS() {
	bloomIntensity = 1.0f;
	
	inputColorTexture.description.state.type = IRender::Resource::TextureDescription::TEXTURE_2D;
	inputBloomTexture0.description.state.type = IRender::Resource::TextureDescription::TEXTURE_2D;
	inputBloomTexture1.description.state.type = IRender::Resource::TextureDescription::TEXTURE_2D;
	inputBloomTexture2.description.state.type = IRender::Resource::TextureDescription::TEXTURE_2D;
}

// https://github.com/TheRealMJP/BakingLab/blob/master/BakingLab/ACES.hlsl

String ScreenFS::GetShaderText() {
	return UnifyShaderCode(
		const float3x3 ACESInputMat = make_float3x3(
			0.59719, 0.07600, 0.02840,
			0.35458, 0.90834, 0.13383,
			0.04823, 0.01566, 0.83777
		);

		const float3x3 ACESOutputMat = make_float3x3(
			1.60475, -0.10208, -0.00327,
			-0.53108, 1.10813, -0.07276,
			-0.07367, -0.00605, 1.07602
		);

		float4 bloomColor = float4(0, 0, 0, 0);
		//bloomColor += texture(inputBloomTexture0, rasterCoord);
		//bloomColor += texture(inputBloomTexture1, rasterCoord);
		//bloomColor += texture(inputBloomTexture2, rasterCoord);

		float3 color = texture(inputColorTexture, rasterCoord).xyz;
		color = mult_vec(ACESInputMat, color + bloomColor.xyz);
		float3 a = color * (color + float3(0.0245786f, 0.0245786f, 0.0245786f)) - float3(0.000090537f, 0.000090537f, 0.000090537f);
		float3 b = color * (color * 0.983729f + float3(0.4329510f, 0.4329510f, 0.4329510f)) + float3(0.238081f, 0.238081f, 0.238081f);
		outputColor.xyz = saturate(mult_vec(ACESOutputMat, a / b));
		outputColor.w = 1;
	);
}

TObject<IReflect>& ScreenFS::operator () (IReflect& reflect) {
	BaseClass::operator () (reflect);

	if (reflect.IsReflectProperty()) {
		ReflectProperty(inputColorTexture);
		ReflectProperty(inputBloomTexture0);
		ReflectProperty(inputBloomTexture1);
		ReflectProperty(inputBloomTexture2);
		ReflectProperty(rasterCoord)[IShader::BindInput(IShader::BindInput::TEXCOORD)];
		ReflectProperty(outputColor)[IShader::BindOutput(IShader::BindOutput::COLOR)];

		ReflectProperty(bloomIntensity)[IShader::BindConst<float>()];
	}

	return *this;
}
