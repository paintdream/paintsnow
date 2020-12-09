#define USE_SWIZZLE
#include "LightEncoderCS.h"
#include "../../../../General/Template/TShaderMacro.h"

using namespace PaintsNow;

LightEncoderCS::LightEncoderCS() : computeGroup(4, 4, 1) {
	depthTexture.description.state.type = IRender::Resource::TextureDescription::TEXTURE_2D;
	depthTexture.description.state.format = IRender::Resource::TextureDescription::HALF;
	depthTexture.description.state.layout = IRender::Resource::TextureDescription::RG;
	depthTexture.memorySpec = IShader::READONLY;
	lightInfoBuffer.description.usage = IRender::Resource::BufferDescription::UNIFORM;

	lightInfos.resize(MAX_LIGHT_COUNT);
	lightBuffer.description.usage = IRender::Resource::BufferDescription::STORAGE; // SSBO
}

String LightEncoderCS::GetShaderText() {
	return UnifyShaderCode(
		uint3 id = WorkGroupID * WorkGroupSize + LocalInvocationID;
		float2 depthRange = imageLoad(depthTexture, int2(id.x, id.y)).xy;
		float2 rasterCoord = float2((float(id.x) + 0.5) * screenSize.x, (float(id.y) + 0.5) * screenSize.y);
		float4 farPosition = float4(rasterCoord.x, rasterCoord.y, depthRange.x, 1) * float(2.0) - float4(1.0, 1.0, 1.0, 1.0);
		farPosition = mult_vec(inverseProjectionMatrix, farPosition);
		farPosition /= farPosition.w;
		float4 nearPosition = float4(rasterCoord.x, rasterCoord.y, depthRange.y, 1) * float(2.0) - float4(1.0, 1.0, 1.0, 1.0);
		nearPosition = mult_vec(inverseProjectionMatrix, nearPosition);
		nearPosition /= nearPosition.w;

		uint offset = (id.x + id.y * NumWorkGroups.x) * 64;
		uint count = min(uint(lightCount), uint(255));
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
				lightBufferData[offset++] = packedLightID;
				j = 0;
			}
		}

		// last commit
		if (j != 0) {
			lightBufferData[offset++] = packedLightID;
		}
	);
}

TObject<IReflect>& LightEncoderCS::operator () (IReflect& reflect) {
	BaseClass::operator () (reflect);

	if (reflect.IsReflectProperty()) {
		ReflectProperty(computeGroup)[BindInput(BindInput::COMPUTE_GROUP)];
		ReflectProperty(depthTexture);
		ReflectProperty(lightInfoBuffer);
		ReflectProperty(lightBuffer);

		ReflectProperty(inverseProjectionMatrix)[lightInfoBuffer][BindInput(BindInput::TRANSFORM_VIEWPROJECTION_INV)];
		ReflectProperty(screenSize)[lightInfoBuffer][BindInput(BindInput::GENERAL)];
		ReflectProperty(lightCount)[lightInfoBuffer][BindInput(BindInput::GENERAL)];
		ReflectProperty(reserved)[lightInfoBuffer][BindInput(BindInput::GENERAL)];
		ReflectProperty(lightInfos)[lightInfoBuffer][BindInput(BindInput::GENERAL)];

		ReflectProperty(lightBufferData)[lightBuffer][BindInput(BindInput::GENERAL)];
	}

	return *this;
}
