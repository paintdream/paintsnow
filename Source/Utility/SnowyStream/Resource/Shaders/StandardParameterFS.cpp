#define USE_SWIZZLE
#include "StandardParameterFS.h"
#include "../../../../General/Template/TShaderMacro.h"

using namespace PaintsNow;

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
		ReflectProperty(baseColorTexture)[BindInput(BindInput::MAINTEXTURE)];
		ReflectProperty(normalTexture);
		ReflectProperty(mixtureTexture);

		ReflectProperty(enableBaseColorTexture)[BindConst<bool>()];
		ReflectProperty(enableNormalTexture)[BindConst<bool>()];
		ReflectProperty(enableMaterialTexture)[BindConst<bool>()];

		/*
		ReflectProperty(paramBuffer);
		ReflectProperty(invScreenSize)[paramBuffer];
		ReflectProperty(timestamp)[paramBuffer];
		ReflectProperty(reserved)[paramBuffer];*/

		ReflectProperty(texCoord)[BindInput(BindInput::TEXCOORD)];
		ReflectProperty(viewNormal)[BindInput(BindInput::TEXCOORD)];
		ReflectProperty(viewTangent)[BindInput(BindInput::TEXCOORD)];
		ReflectProperty(viewBinormal)[BindInput(BindInput::TEXCOORD)];
		ReflectProperty(tintColor)[BindInput(BindInput::TEXCOORD)];

		ReflectProperty(outputColor)[BindOutput(BindOutput::LOCAL)];
		ReflectProperty(outputNormal)[BindOutput(BindOutput::LOCAL)];
		ReflectProperty(alpha)[BindOutput(BindOutput::LOCAL)];
		ReflectProperty(metallic)[BindOutput(BindOutput::LOCAL)];
		ReflectProperty(roughness)[BindOutput(BindOutput::LOCAL)];
		ReflectProperty(occlusion)[BindOutput(BindOutput::LOCAL)];
	}

	return *this;
}
