#define USE_SWIZZLE
#include "ShadowMaskFS.h"
#include "../../../../General/Template/TShaderMacro.h"

using namespace PaintsNow;
using namespace PaintsNow::NsSnowyStream;
using namespace PaintsNow::ShaderMacro;

ShadowMaskFS::ShadowMaskFS() {
	uniformBuffer.description.usage = IRender::Resource::BufferDescription::UNIFORM;
}

String ShadowMaskFS::GetShaderText() {
	return UnifyShaderCode(
	float depth = textureLod(depthTexture, rasterCoord, float(0)).x;
	float4 position = float4(rasterCoord.x, rasterCoord.y, depth, 1);
	position = position * float(2) - float4(1, 1, 1, 1);
	position = mult_vec(reprojectionMatrix, position);
	position.xyz /= position.w;
	float refDepth = textureLod(shadowTexture, position.xy * float(0.5) + float2(0.5, 0.5), float(0)).x;
	shadow = float4(refDepth > position.z * 0.5 + 0.5 ? 1 : 0, 0, 0, 0);
	);
}

TObject<IReflect>& ShadowMaskFS::operator () (IReflect& reflect) {
	BaseClass::operator () (reflect);

	if (reflect.IsReflectProperty()) {
		ReflectProperty(shadowTexture);
		ReflectProperty(depthTexture);
		ReflectProperty(uniformBuffer);

		ReflectProperty(rasterCoord)[IShader::BindInput(IShader::BindInput::TEXCOORD)];
		ReflectProperty(reprojectionMatrix)[uniformBuffer][IShader::BindInput(IShader::BindInput::GENERAL)];
		ReflectProperty(shadow)[IShader::BindOutput(IShader::BindOutput::COLOR)];
	}

	return *this;
}
