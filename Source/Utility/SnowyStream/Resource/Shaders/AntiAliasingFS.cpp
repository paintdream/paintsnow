#define USE_SWIZZLE
#include "AntiAliasingFS.h"
#include "../../../../General/Template/TShaderMacro.h"

using namespace PaintsNow;
using namespace PaintsNow::NsSnowyStream;
using namespace PaintsNow::ShaderMacro;

AntiAliasingFS::AntiAliasingFS() : lastRatio(0.75f) {
	inputTexture.description.state.type = IRender::Resource::TextureDescription::TEXTURE_2D;
	depthTexture.description.state.type = IRender::Resource::TextureDescription::TEXTURE_2D;
	lastInputTexture.description.state.type = IRender::Resource::TextureDescription::TEXTURE_2D;
	uniformBuffer.description.usage = IRender::Resource::BufferDescription::UNIFORM;
}

String AntiAliasingFS::GetShaderText() {
	return UnifyShaderCode(
		float depth = texture(depthTexture, rasterCoord.xy).x;
		outputColor = texture(inputTexture, rasterCoord.xy + unjitter);
		float4 homoPos = float4(rasterCoord.x, rasterCoord.y, depth, 1) * float(2) - float4(1.0, 1.0, 1.0, 1.0);
		homoPos = mult_vec(reprojectionMatrix, homoPos);
		float4 lastColor = texture(lastInputTexture, homoPos.xy / homoPos.w * float(0.5) + float2(0.5, 0.5));

		float4 n1 = texture(inputTexture, rasterCoord.xy + float2(invScreenSize.x, 0));
		float4 n2 = texture(inputTexture, rasterCoord.xy + float2(-invScreenSize.x, 0));
		float4 n3 = texture(inputTexture, rasterCoord.xy + float2(0, invScreenSize.y));
		float4 n4 = texture(inputTexture, rasterCoord.xy + float2(0, -invScreenSize.y));
		float4 n5 = texture(inputTexture, rasterCoord.xy + float2(invScreenSize.x, invScreenSize.y));
		float4 n6 = texture(inputTexture, rasterCoord.xy + float2(-invScreenSize.x, invScreenSize.y));
		float4 n7 = texture(inputTexture, rasterCoord.xy + float2(invScreenSize.x, -invScreenSize.y));
		float4 n8 = texture(inputTexture, rasterCoord.xy + float2(-invScreenSize.x, -invScreenSize.y));

		float4 avg = (n1 + n2 + n3 + n4 + n5 + n6 + n7 + n8 + outputColor) / float(9);
		float4 avgsq = (n1 * n1 + n2 * n2 + n3 * n3 + n4 * n4 + n5 * n5 + n6 * n6 + n7 * n7 + n8 * n8 + outputColor * outputColor) / float(9);
		float4 sigma = sqrt(max(avgsq - avg * avg, float4(0, 0, 0, 0)));
		float4 minColor = avg - sigma * float(3);
		float4 maxColor = avg + sigma * float(3);

		outputColor = lerp(outputColor, clamp(lastColor, minColor, maxColor), lastRatio);
	);
}

TObject<IReflect>& AntiAliasingFS::operator () (IReflect& reflect) {
	BaseClass::operator () (reflect);

	if (reflect.IsReflectProperty()) {
		// inputs
		ReflectProperty(inputTexture);
		ReflectProperty(lastInputTexture);
		ReflectProperty(depthTexture);
		ReflectProperty(uniformBuffer);
		ReflectProperty(rasterCoord)[IShader::BindInput(IShader::BindInput::TEXCOORD)];
		ReflectProperty(outputColor)[IShader::BindOutput(IShader::BindOutput::COLOR)];

		ReflectProperty(reprojectionMatrix)[uniformBuffer];
		ReflectProperty(invScreenSize)[uniformBuffer];
		ReflectProperty(unjitter)[uniformBuffer];
		ReflectProperty(lastRatio)[uniformBuffer];
	}

	return *this;
}
