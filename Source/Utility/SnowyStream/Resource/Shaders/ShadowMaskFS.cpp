#define USE_SWIZZLE
#include "ShadowMaskFS.h"
#include "../../../../General/Template/TShaderMacro.h"

using namespace PaintsNow;

ShadowMaskFS::ShadowMaskFS() {
	uniformBuffer.description.usage = IRender::Resource::BufferDescription::UNIFORM;
}

String ShadowMaskFS::GetShaderText() {
	return UnifyShaderCode(
	rasterCoord.xy = rasterCoord.xy * invScreenSize.xy;
	float depth = textureLod(depthTexture, rasterCoord.xy, float(0)).x;
	float4 position = float4(rasterCoord.x, rasterCoord.y, depth, 1);
	position = position * float(2) - float4(1, 1, 1, 1);
	position = mult_vec(reprojectionMatrix, position);
	position.xyz = position.xyz / position.w;
	float3 uv = abs(position.xyz);
	clip(float(uv.x < 1 && uv.y < 1 && uv.z < 1) - float(0.5));

	position.xyz = position.xyz * float(0.5) + float3(0.5, 0.5, 0.5);
	float refDepth = textureLod(shadowTexture, position.xy, float(0)).x;
	shadow = float4(step(position.z + 0.00005, refDepth), 0, 0, 0);
	);
}

TObject<IReflect>& ShadowMaskFS::operator () (IReflect& reflect) {
	BaseClass::operator () (reflect);

	if (reflect.IsReflectProperty()) {
		ReflectProperty(shadowTexture);
		ReflectProperty(depthTexture);
		ReflectProperty(uniformBuffer);

		ReflectProperty(rasterCoord)[BindInput(BindInput::RASTERCOORD)];
		ReflectProperty(reprojectionMatrix)[uniformBuffer][BindInput(BindInput::GENERAL)];
		ReflectProperty(invScreenSize)[uniformBuffer][BindInput(BindInput::GENERAL)];
		ReflectProperty(shadow)[BindOutput(BindOutput::COLOR)];
	}

	return *this;
}
