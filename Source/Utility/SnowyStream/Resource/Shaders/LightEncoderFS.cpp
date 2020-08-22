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
		float4 masks[8];
		masks[0] = float4(1.0, 0.0, 0.0, 0.0);
		masks[1] = float4(256.0, 0.0, 0.0, 0.0);
		masks[2] = float4(0.0, 1.0, 0.0, 0.0);
		masks[3] = float4(0.0, 256.0, 0.0, 0.0);
		masks[4] = float4(0.0, 0.0, 1.0, 0.0);
		masks[5] = float4(0.0, 0.0, 256.0, 0.0);
		masks[6] = float4(0.0, 0.0, 0.0, 1.0);
		masks[7] = float4(0.0, 0.0, 0.0, 256.0);

		float2 depthRange = texture(depthTexture, rasterCoord.xy).xy;
		float4 nearPosition = float4(rasterCoord.x, rasterCoord.y, depthRange.x, 1) * float(2.0) - float4(1.0, 1.0, 1.0, 1.0);
		float4 farPosition = float4(rasterCoord.x, rasterCoord.y, depthRange.y, 1) * float(2.0) - float4(1.0, 1.0, 1.0, 1.0);
		nearPosition = mult_vec(inverseProjectionMatrix, nearPosition);
		nearPosition /= nearPosition.w;
		farPosition = mult_vec(inverseProjectionMatrix, farPosition);
		farPosition /= farPosition.w;

		outputIndex = float4(0, 0, 0, 0);
		int j = 0;
		int count = min(int(lightCount), 256);
		for (int i = 0; i < count; i++) {
			float4 lightInfo = lightInfos[i];
			if (lightInfo.w < 0.025) {
				outputIndex += masks[j++] * float(i + 1);
			} else {
				float3 L = lightInfo.xyz - nearPosition.xyz;
				float3 F = lightInfo.xyz - farPosition.xyz;
				float3 P = normalize(farPosition.xyz - nearPosition.xyz);
				float PoL = dot(P, L);
				float PoF = dot(P, F);
				float r = dot(L, L) - PoL * PoL;
				if (r <= lightInfo.w && (PoL * PoF <= 0 || dot(L, L) <= lightInfo.w || dot(F, F) <= lightInfo.w)) {
					if (j < 8) {
						outputIndex += masks[j++] * float(i + 1);
					} else {
						break;
					}
				}
			}
		}

		outputIndex = outputIndex / float(65535.0);
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
