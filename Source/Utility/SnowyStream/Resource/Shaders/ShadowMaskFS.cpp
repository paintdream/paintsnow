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
	position.xyz = position.xyz / position.w;
	float3 uv = abs(position.xyz);
	if (uv.x >= 1 || uv.y >= 1 || uv.z >= 1) {
		shadow = float4(0, 0, 0, 0);
	} else {
		position.xyz = position.xyz * float(0.5) + float3(0.5, 0.5, 0.5);
		float refDepth = textureLod(shadowTexture, position.xy, float(0)).x;
		shadow = float4(step(refDepth, position.z + 0.001), 0, 0, 0);
	}
	);
}

TObject<IReflect>& ShadowMaskFS::operator () (IReflect& reflect) {
	BaseClass::operator () (reflect);

	if (reflect.IsReflectProperty()) {
		ReflectProperty(shadowTexture);
		ReflectProperty(depthTexture);
		ReflectProperty(uniformBuffer);

		ReflectProperty(rasterCoord)[BindInput(BindInput::TEXCOORD)];
		ReflectProperty(reprojectionMatrix)[uniformBuffer][BindInput(BindInput::GENERAL)];
		ReflectProperty(shadow)[BindOutput(BindOutput::COLOR)];
	}

	return *this;
}
