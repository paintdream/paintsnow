#define USE_SWIZZLE
#include "StandardLightingFS.h"
#include "../../../../General/Template/TShaderMacro.h"

using namespace PaintsNow;
using namespace PaintsNow::NsSnowyStream;
using namespace PaintsNow::ShaderMacro;

StandardLightingFS::StandardLightingFS() : lightCount(0), cubeLevelInv(0) {
	specTexture.description.state.type = IRender::Resource::TextureDescription::TEXTURE_2D_CUBE;
	lightBuffer.description.usage = IRender::Resource::BufferDescription::UNIFORM;
	paramBuffer.description.usage = IRender::Resource::BufferDescription::UNIFORM;

	lightInfos.resize(MAX_LIGHT_COUNT * 2);
}

String StandardLightingFS::GetShaderText() {
	return UnifyShaderCode(
	const float3 MINSPEC = float3(0.04, 0.04, 0.04);
	baseColor = pow(baseColor, float3(GAMMA, GAMMA, GAMMA));
	float3 diff = (baseColor - baseColor * metallic) / PI;
	float3 spec = lerp(MINSPEC, baseColor, metallic);
	float3 V = -normalize(viewPosition);
	float3 N = viewNormal;
	float NoV = saturate(dot(V, N));

	float3 R = mult_vec(float3x3(invWorldNormalMatrix), reflect(V, N));
	float3 env = textureLod(specTexture, R, roughness * cubeLevelInv).xyz;
	float4 r = float4(-1, -0.0275, -0.572, 0.022) * roughness + float4(1, 0.0425, 1.04, -0.04);
	float a = min(r.x * r.x, exp2(-9.28 * NoV)) * r.x + r.y;
	float2 AB = float2(-1.04, 1.04) * a + r.zw;
	env = env * AB.x + AB.y;
	mainColor = float4(0, 0, 0, 1);
	// mainColor.xyz += diff.xyz * float3(1.0, 1.0, 1.0); // ambient
	mainColor.xyz += env * spec;

	float4 idx = texture(lightTexture, rasterCoord.xy);
	float f = saturate(50.0 * spec.y);
	float p = roughness * roughness;
	p = p * p;

	for (int k = 0; k < 8; k++) {
		float v = idx[k / 2];
		int i = int(v * 65535);
		i = k % 2 == 0 ? i - (i / 256) * 256 : i / 256;
		if (i == 0) break;

		float4 pos = lightInfos[i * 2 - 2];
		float4 color = lightInfos[i * 2 - 1];
		if (pos.w + shadow < 0.5) continue;

		float3 L = pos.xyz - viewPosition.xyz * pos.www;
		float s = saturate(1.0 / (0.001 + dot(L, L) * color.w));
		color.xyz = color.xyz * s;
		L = normalize(L);
		float3 H = normalize(L + V);
		float NoH = saturate(dot(N, H));
		float NoL = saturate(dot(N, L));
		float VoH = saturate(dot(V, H));
		float q = (NoH * p - NoH) * NoH + 1.0;
		float D = p / max(q * q, 0.0001);
		float2 vl = clamp(float2(NoV, NoL), float2(0.01, 0.1), float2(1, 1));
		vl = vl.yx * sqrt(saturate(-vl * p + vl) * vl + p);
		float G = (0.5 / PI) / max(vl.x + vl.y, 0.0001);
		float e = exp2(VoH * (-5.55473 * VoH - 6.98316));
		float3 F = spec + (float3(f, f, f) - spec) * e;

		mainColor.xyz += (diff + F * (D * G)) * color.xyz * NoL;
	}
	mainColor.xyz = pow(max(mainColor.xyz, float3(0, 0, 0)), float3(1.0, 1.0, 1.0) / GAMMA);
	// mainColor.xyz = mainColor.xyz * float(0.0001) + float3(1 - shadow, 1 - shadow, 1 - shadow);
);
}

TObject<IReflect>& StandardLightingFS::operator () (IReflect& reflect) {
	BaseClass::operator () (reflect);

	if (reflect.IsReflectProperty()) {
		ReflectProperty(lightTexture);
		ReflectProperty(specTexture);
		ReflectProperty(lightBuffer);
		ReflectProperty(paramBuffer);

		ReflectProperty(viewPosition)[IShader::BindInput(IShader::BindInput::LOCAL)];
		ReflectProperty(viewNormal)[IShader::BindInput(IShader::BindInput::LOCAL)];
		ReflectProperty(baseColor)[IShader::BindInput(IShader::BindInput::LOCAL)];
		ReflectProperty(metallic)[IShader::BindInput(IShader::BindInput::LOCAL)];
		ReflectProperty(roughness)[IShader::BindInput(IShader::BindInput::LOCAL)];
		ReflectProperty(shadow)[IShader::BindInput(IShader::BindInput::LOCAL)];
		ReflectProperty(occlusion)[IShader::BindInput(IShader::BindInput::LOCAL)];
		ReflectProperty(invWorldNormalMatrix)[paramBuffer][IShader::BindInput(IShader::BindInput::GENERAL)];
		ReflectProperty(cubeLevelInv)[paramBuffer][IShader::BindInput(IShader::BindInput::GENERAL)];
		ReflectProperty(lightInfos)[lightBuffer][IShader::BindInput(IShader::BindInput::GENERAL)];
		ReflectProperty(mainColor)[IShader::BindOutput(IShader::BindOutput::COLOR)];
	}
	
	return *this;
}
