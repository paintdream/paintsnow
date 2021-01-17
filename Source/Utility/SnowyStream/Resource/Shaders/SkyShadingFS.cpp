#define USE_SWIZZLE
#include "SkyShadingFS.h"
#include "../../../../General/Template/TShaderMacro.h"

using namespace PaintsNow;

SkyShadingFS::SkyShadingFS() {
	paramBuffer.description.usage = IRender::Resource::BufferDescription::UNIFORM;
	transmittanceTexture.description.state.type = IRender::Resource::TextureDescription::TEXTURE_2D;
	abstractScatteringTexture.description.state.type = IRender::Resource::TextureDescription::TEXTURE_3D;
	reducedScatteringTexture.description.state.type = IRender::Resource::TextureDescription::TEXTURE_3D;
	scatteringTexture.description.state.type = IRender::Resource::TextureDescription::TEXTURE_3D;
	scatteringDensityTexture.description.state.type = IRender::Resource::TextureDescription::TEXTURE_3D;
	irradianceTexture.description.state.type = IRender::Resource::TextureDescription::TEXTURE_2D;
}

String SkyShadingFS::GetShaderText() {
	return UnifyShaderCode(
		outputColor = float4(1.0, 1.0, 1.0, 1.0);
	);
}

TObject<IReflect>& SkyShadingFS::operator () (IReflect& reflect) {
	BaseClass::operator () (reflect);

	if (reflect.IsReflectProperty()) {
		ReflectProperty(paramBuffer);
		ReflectProperty(worldPosition)[BindInput(BindInput::TEXCOORD)];
		ReflectProperty(outputColor)[BindOutput(BindOutput::COLOR)];
	}

	return *this;
}