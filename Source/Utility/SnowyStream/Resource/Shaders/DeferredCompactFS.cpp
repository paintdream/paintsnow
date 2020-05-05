#define USE_SWIZZLE
#include "DeferredCompactFS.h"
#include "../../../../General/Template/TShaderMacro.h"

using namespace PaintsNow;
using namespace PaintsNow::NsSnowyStream;
using namespace PaintsNow::ShaderMacro;

DeferredCompactEncodeFS::DeferredCompactEncodeFS() {
}

String DeferredCompactEncodeFS::GetShaderText() {
	return UnifyShaderCode(
		encodeNormalRoughnessMetallic.xy = outputNormal.xy * float(127.0 / 255.0) + float2(128.0 / 255.0, 128.0 / 255.0);
		encodeNormalRoughnessMetallic.z = roughness;
		encodeNormalRoughnessMetallic.w = metallic;
		encodeBaseColorOcclusion.xyz = outputColor.xyz;
		encodeBaseColorOcclusion.w = occlusion;
	);
}

TObject<IReflect>& DeferredCompactEncodeFS::operator () (IReflect& reflect) {
	BaseClass::operator () (reflect);

	if (reflect.IsReflectProperty()) {
		// inputs
		ReflectProperty(outputColor)[IShader::BindInput(IShader::BindInput::LOCAL)];
		ReflectProperty(outputNormal)[IShader::BindInput(IShader::BindInput::LOCAL)];
		ReflectProperty(occlusion)[IShader::BindInput(IShader::BindInput::LOCAL)];
		ReflectProperty(metallic)[IShader::BindInput(IShader::BindInput::LOCAL)];
		ReflectProperty(roughness)[IShader::BindInput(IShader::BindInput::LOCAL)];

		ReflectProperty(encodeBaseColorOcclusion)[IShader::BindOutput(IShader::BindOutput::COLOR)];
		ReflectProperty(encodeNormalRoughnessMetallic)[IShader::BindOutput(IShader::BindOutput::COLOR)];
	}

	return *this;
}

DeferredCompactDecodeFS::DeferredCompactDecodeFS() {
	BaseColorOcclusionTexture.description.state.type = IRender::Resource::TextureDescription::TEXTURE_2D;
	NormalRoughnessMetallicTexture.description.state.type = IRender::Resource::TextureDescription::TEXTURE_2D;
	DepthTexture.description.state.type = IRender::Resource::TextureDescription::TEXTURE_2D;
	uniformProjectionBuffer.description.usage = IRender::Resource::BufferDescription::UNIFORM;
}

String DeferredCompactDecodeFS::GetShaderText() {
	return UnifyShaderCode(
		depth = textureLod(DepthTexture, rasterCoord.xy, float(0)).x;
		float4 NormalRoughnessMetallic = textureLod(NormalRoughnessMetallicTexture, rasterCoord.xy, float(0));
		float4 BaseColorOcclusion = textureLod(BaseColorOcclusionTexture, rasterCoord.xy, float(0));
		shadow = textureLod(ShadowTexture, rasterCoord.xy, float(0)).x;

		float2 nn = NormalRoughnessMetallic.xy * float(255.0 / 127.0) - float2(128.0 / 127.0, 128.0 / 127.0);
		viewNormal = float3(nn.x, nn.y, sqrt(max(0.0, 1 - dot(nn.xy, nn.xy))));
		roughness = NormalRoughnessMetallic.z;
		metallic = NormalRoughnessMetallic.w;

		baseColor = BaseColorOcclusion.xyz;
		occlusion = BaseColorOcclusion.w;

		float4 position = float4(rasterCoord.x, rasterCoord.y, depth, 1);
		position.xyz = position.xyz * float3(2, 2, 2) - float3(1, 1, 1);
		position = mult_vec(inverseProjectionMatrix, position);
		viewPosition = position.xyz / position.w;
	);
}

TObject<IReflect>& DeferredCompactDecodeFS::operator () (IReflect& reflect) {
	BaseClass::operator () (reflect);

	if (reflect.IsReflectProperty()) {
		ReflectProperty(BaseColorOcclusionTexture);
		ReflectProperty(NormalRoughnessMetallicTexture);
		ReflectProperty(DepthTexture);
		ReflectProperty(ShadowTexture);
		ReflectProperty(uniformProjectionBuffer);

		ReflectProperty(inverseProjectionMatrix)[uniformProjectionBuffer][IShader::BindInput(IShader::BindInput::TRANSFORM_VIEWPROJECTION_INV)];
		ReflectProperty(rasterCoord)[IShader::BindInput(IShader::BindInput::TEXCOORD)];
		
		ReflectProperty(viewPosition)[IShader::BindOutput(IShader::BindOutput::LOCAL)];
		ReflectProperty(viewNormal)[IShader::BindOutput(IShader::BindOutput::LOCAL)];
		ReflectProperty(baseColor)[IShader::BindOutput(IShader::BindOutput::LOCAL)];
		ReflectProperty(depth)[IShader::BindOutput(IShader::BindOutput::LOCAL)];
		ReflectProperty(occlusion)[IShader::BindOutput(IShader::BindOutput::LOCAL)];
		ReflectProperty(metallic)[IShader::BindOutput(IShader::BindOutput::LOCAL)];
		ReflectProperty(roughness)[IShader::BindOutput(IShader::BindOutput::LOCAL)];
		ReflectProperty(shadow)[IShader::BindOutput(IShader::BindOutput::LOCAL)];
	}

	return *this;
}