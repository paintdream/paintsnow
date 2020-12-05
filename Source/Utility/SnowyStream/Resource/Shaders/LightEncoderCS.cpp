#define USE_SWIZZLE
#include "LightEncoderCS.h"
#include "../../../../General/Template/TShaderMacro.h"

using namespace PaintsNow;

LightEncoderCS::LightEncoderCS() {
	depthTexture.description.state.type = IRender::Resource::TextureDescription::TEXTURE_2D;
	lightBuffer.description.usage = IRender::Resource::BufferDescription::UNIFORM;

	lightInfos.resize(MAX_LIGHT_COUNT);
	encodeBuffer.description.usage = IRender::Resource::BufferDescription::STORAGE; // SSBO
}

String LightEncoderCS::GetShaderText() {
	return UnifyShaderCode(
		UInt3 id = WorkGroupID * WorkGroupSize + LocalInvocationID;
		float2 depthRange = imageLoad(depthTexture, id).xy;
		float2 rasterCoord = float2((id.x + 0.5) * screenSize.x, (id.y + 0.5) * screenSize.y);
		float4 farPosition = float4((rasterCoord.x + 0.5) * screenSize.x, (rasterCoord.y + 0.5) * screenSize.x, depthRange.x, 1) * float(2.0) - float4(1.0, 1.0, 1.0, 1.0);
		farPosition = mult_vec(inverseProjectionMatrix, farPosition);
		farPosition /= farPosition.w;
		float4 nearPosition = float4(rasterCoord.x, rasterCoord.y, depthRange.y, 1) * float(2.0) - float4(1.0, 1.0, 1.0, 1.0);
		nearPosition = mult_vec(inverseProjectionMatrix, nearPosition);
		nearPosition /= nearPosition.w;

		uint offset = (id.x + id.y * NumWorkGroups.x) * 64;
		uint count = min(int(lightCount), 255);
		uint packedLightID = 0;
		uint j = 0;
		for (uint i = 0; i < count; i++) {
			float4 lightInfo = lightInfos[i];
			if (lightInfo.w == 0.0) {
				packedLightID = (packedLightID << 8) + (i + 1);
				j++;
			} else {
				float3 L = lightInfo.xyz - nearPosition.xyz;
				float3 F = lightInfo.xyz - farPosition.xyz;
				float3 P = normalize(farPosition.xyz - nearPosition.xyz);
				float PoL = dot(P, L);
				float PoF = dot(P, F);
				float r = dot(L, L) - PoL * PoL;
				if (r <= lightInfo.w && (PoL * PoF <= 0 || dot(L, L) <= lightInfo.w || dot(F, F) <= lightInfo.w)) {
					packedLightID = (packedLightID << 8) + (i + 1);
					j++;
				}
			}

			if (j == 4) {
				// commit
				encodeBufferData[offset++] = packedLightID;
				j = 0;
			}
		}

		// last commit
		if (j != 0) {
			encodeBufferData[offset++] = packedLightID;
		}
	);
}

TObject<IReflect>& LightEncoderCS::operator () (IReflect& reflect) {
	BaseClass::operator () (reflect);

	if (reflect.IsReflectProperty()) {
		ReflectProperty(depthTexture);
		ReflectProperty(lightBuffer);
		ReflectProperty(encodeBuffer);

		ReflectProperty(inverseProjectionMatrix)[lightBuffer][BindInput(BindInput::TRANSFORM_VIEWPROJECTION_INV)];
		ReflectProperty(screenSize)[lightBuffer][BindInput(BindInput::GENERAL)];
		ReflectProperty(lightCount)[lightBuffer][BindInput(BindInput::GENERAL)];
		ReflectProperty(reserved)[lightBuffer][BindInput(BindInput::GENERAL)];
		ReflectProperty(lightInfos)[lightBuffer][BindInput(BindInput::GENERAL)];

		ReflectProperty(encodeBufferData)[encodeBuffer][BindOutput(BindOutput::COLOR)];
	}

	return *this;
}
