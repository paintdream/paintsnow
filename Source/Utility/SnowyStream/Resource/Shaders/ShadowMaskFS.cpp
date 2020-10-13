#define USE_SWIZZLE
#include "ShadowMaskFS.h"
#include "../../../../General/Template/TShaderMacro.h"

using namespace PaintsNow;

ShadowMaskFS::ShadowMaskFS() {
	uniformBuffer.description.usage = IRender::Resource::BufferDescription::UNIFORM;
}

String ShadowMaskFS::GetShaderText() {
	return UnifyShaderCode(
	float depth = textureLod(depthTexture, rasterCoord, float(0)).x;
	float4 position = float4(rasterCoord.x, rasterCoord.y, depth, 1);
	position = position * float(2) - float4(1, 1, 1, 1);
	float4 p;
	float3 uv;
	p = mult_vec(reprojectionMatrix0, position);
	p.xyz = p.xyz / p.w;
	uv = abs(p.xyz);

	if (uv.x < 1 && uv.y < 1 && uv.z < 1) {
		p.xyz = p.xyz * float(0.5) + float3(0.5, 0.5, 0.5);
		float refDepth = textureLod(shadowTexture0, p.xy, float(0)).x;
		shadow = float4(step(p.z + 0.001, refDepth), 0, 0, 0);
	} else {
		p = mult_vec(reprojectionMatrix1, position);
		p.xyz = p.xyz / p.w;
		uv = abs(p.xyz);

		if (uv.x < 1 && uv.y < 1 && uv.z < 1) {
			p.xyz = p.xyz * float(0.5) + float3(0.5, 0.5, 0.5);
			float refDepth = textureLod(shadowTexture1, p.xy, float(0)).x;
			shadow = float4(step(p.z + 0.001, refDepth), 0, 0, 0);
		} else {
			p = mult_vec(reprojectionMatrix2, position);
			p.xyz = p.xyz / p.w;
			uv = abs(p.xyz);

			if (uv.x < 1 && uv.y < 1 && uv.z < 1) {
				p.xyz = p.xyz * float(0.5) + float3(0.5, 0.5, 0.5);
				float refDepth = textureLod(shadowTexture1, p.xy, float(0)).x;
				shadow = float4(step(p.z + 0.001, refDepth), 0, 0, 0);
			} else {
				shadow = float4(0, 0, 0, 0);
			}
		}
	}
	);
}

TObject<IReflect>& ShadowMaskFS::operator () (IReflect& reflect) {
	BaseClass::operator () (reflect);

	if (reflect.IsReflectProperty()) {
		ReflectProperty(shadowTexture0);
		ReflectProperty(shadowTexture1);
		ReflectProperty(shadowTexture2);
		ReflectProperty(depthTexture);
		ReflectProperty(uniformBuffer);

		ReflectProperty(rasterCoord)[BindInput(BindInput::TEXCOORD)];
		ReflectProperty(reprojectionMatrix0)[uniformBuffer][BindInput(BindInput::GENERAL)];
		ReflectProperty(reprojectionMatrix1)[uniformBuffer][BindInput(BindInput::GENERAL)];
		ReflectProperty(reprojectionMatrix2)[uniformBuffer][BindInput(BindInput::GENERAL)];
		ReflectProperty(shadow)[BindOutput(BindOutput::COLOR)];
	}

	return *this;
}
