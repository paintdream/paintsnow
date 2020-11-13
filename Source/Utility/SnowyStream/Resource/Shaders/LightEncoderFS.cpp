#define USE_SWIZZLE
#include "LightEncoderFS.h"
#include "../../../../General/Template/TShaderMacro.h"

using namespace PaintsNow;

LightEncoderFS::LightEncoderFS() {
	depthTexture.description.state.type = IRender::Resource::TextureDescription::TEXTURE_2D;
	lightBuffer.description.usage = IRender::Resource::BufferDescription::UNIFORM;

	lightInfos.resize(MAX_LIGHT_COUNT);
}

String LightEncoderFS::GetShaderText() {
	return UnifyShaderCode(
		float2 depthRange = texture(depthTexture, rasterCoord.xy).xy;
		float4 farPosition = float4(rasterCoord.x, rasterCoord.y, depthRange.x, 1) * float(2.0) - float4(1.0, 1.0, 1.0, 1.0);
		farPosition = mult_vec(inverseProjectionMatrix, farPosition);
		farPosition /= farPosition.w;
		float4 nearPosition = float4(rasterCoord.x, rasterCoord.y, depthRange.y, 1) * float(2.0) - float4(1.0, 1.0, 1.0, 1.0);
		nearPosition = mult_vec(inverseProjectionMatrix, nearPosition);
		nearPosition /= nearPosition.w;

		int count = min(int(lightCount), 255);
		outputIndex = float4(0, 0, 0, 0);
		for (int i = 0, j = 0; i < count && j < 4; i++) {
			float4 lightInfo = lightInfos[i];
			if (lightInfo.w == 0.0) {
				outputIndex[j++] = i + 1;
			} else {
				float3 L = lightInfo.xyz - nearPosition.xyz;
				float3 F = lightInfo.xyz - farPosition.xyz;
				float3 P = normalize(farPosition.xyz - nearPosition.xyz);
				float PoL = dot(P, L);
				float PoF = dot(P, F);
				float r = dot(L, L) - PoL * PoL;
				if (r <= lightInfo.w && (PoL * PoF <= 0 || dot(L, L) <= lightInfo.w || dot(F, F) <= lightInfo.w)) {
					outputIndex[j++] = i + 1;
				}
			}
		}

		outputIndex = outputIndex / float(255.0);
	);
}

TObject<IReflect>& LightEncoderFS::operator () (IReflect& reflect) {
	BaseClass::operator () (reflect);

	if (reflect.IsReflectProperty()) {
		// inputs
		ReflectProperty(depthTexture);
		ReflectProperty(lightBuffer);
		ReflectProperty(rasterCoord)[BindInput(BindInput::TEXCOORD)];

		ReflectProperty(inverseProjectionMatrix)[lightBuffer][BindInput(BindInput::TRANSFORM_VIEWPROJECTION_INV)];
		ReflectProperty(invScreenSize)[lightBuffer][BindInput(BindInput::GENERAL)];
		ReflectProperty(lightCount)[lightBuffer][BindInput(BindInput::GENERAL)];
		ReflectProperty(reserved)[lightBuffer][BindInput(BindInput::GENERAL)];
		ReflectProperty(lightInfos)[lightBuffer][BindInput(BindInput::GENERAL)];
		ReflectProperty(outputIndex)[BindOutput(BindOutput::COLOR)];
	}

	return *this;
}
