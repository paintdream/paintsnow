#define USE_SWIZZLE
#include "StandardParameterFS.h"
#include "../../../../General/Template/TShaderMacro.h"

using namespace PaintsNow;
using namespace PaintsNow::NsSnowyStream;
using namespace PaintsNow::ShaderMacro;

StandardParameterFS::StandardParameterFS() : enableBaseColorTexture(true), enableNormalTexture(true), enableMaterialTexture(true) {
	baseColorTexture.description.state.type = IRender::Resource::TextureDescription::TEXTURE_2D;
	normalTexture.description.state.type = IRender::Resource::TextureDescription::TEXTURE_2D;
	mixtureTexture.description.state.type = IRender::Resource::TextureDescription::TEXTURE_2D;
}

String StandardParameterFS::GetShaderText() {
	return UnifyShaderCode(
		if (enableBaseColorTexture) {
			float4 color = texture(baseColorTexture, texCoord.xy);
			outputColor = tintColor.xyz * color.xyz;
			alpha = color.w;
		}

		if (enableNormalTexture) {
			float4 bump = texture(normalTexture, texCoord.xy);
			bump.xyz = bump.xyz * float3(2.0, 2.0, 2.0) - float3(1.0, 1.0, 1.0);
			outputNormal = normalize(viewTangent.xyz * bump.x + viewBinormal.xyz * bump.y + viewNormal.xyz * bump.z);
		}

		if (enableMaterialTexture) {
			float4 material = texture(mixtureTexture, texCoord.xy);
			occlusion = material.x;
			roughness = material.y;
			metallic = material.z;
			// emission = 1 - material.w;
		} else {
			metallic = 0.0;
			roughness = 1.0;
			occlusion = 1.0;
			// emission = 0.0;
		}
	);
}

TObject<IReflect>& StandardParameterFS::operator () (IReflect& reflect) {
	BaseClass::operator () (reflect);

	if (reflect.IsReflectProperty()) {
		ReflectProperty(baseColorTexture);
		ReflectProperty(normalTexture);
		ReflectProperty(mixtureTexture);

		ReflectProperty(enableBaseColorTexture)[IShader::BindConst<bool>()];
		ReflectProperty(enableNormalTexture)[IShader::BindConst<bool>()];
		ReflectProperty(enableMaterialTexture)[IShader::BindConst<bool>()];

		/*
		ReflectProperty(paramBuffer);
		ReflectProperty(invScreenSize)[paramBuffer];
		ReflectProperty(timestamp)[paramBuffer];
		ReflectProperty(reserved)[paramBuffer];*/

		ReflectProperty(texCoord)[IShader::BindInput(IShader::BindInput::TEXCOORD)];
		ReflectProperty(viewNormal)[IShader::BindInput(IShader::BindInput::TEXCOORD)];
		ReflectProperty(viewTangent)[IShader::BindInput(IShader::BindOutput::TEXCOORD)];
		ReflectProperty(viewBinormal)[IShader::BindInput(IShader::BindOutput::TEXCOORD)];
		ReflectProperty(tintColor)[IShader::BindInput(IShader::BindOutput::TEXCOORD)];

		ReflectProperty(outputColor)[IShader::BindOutput(IShader::BindOutput::LOCAL)];
		ReflectProperty(outputNormal)[IShader::BindOutput(IShader::BindOutput::LOCAL)];
		ReflectProperty(alpha)[IShader::BindOutput(IShader::BindOutput::LOCAL)];
		ReflectProperty(metallic)[IShader::BindOutput(IShader::BindOutput::LOCAL)];
		ReflectProperty(roughness)[IShader::BindOutput(IShader::BindOutput::LOCAL)];
		ReflectProperty(occlusion)[IShader::BindOutput(IShader::BindOutput::LOCAL)];
	}

	return *this;
}
