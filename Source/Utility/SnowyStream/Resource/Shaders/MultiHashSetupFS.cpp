#define USE_SWIZZLE
#include "MultiHashSetupFS.h"
#include "../../../../General/Template/TShaderMacro.h"

using namespace PaintsNow;
using namespace PaintsNow::NsSnowyStream;
using namespace PaintsNow::ShaderMacro;

MultiHashSetupFS::MultiHashSetupFS() {
	
}

TObject<IReflect>& MultiHashSetupFS::operator () (IReflect& reflect) {
	BaseClass::operator () (reflect);

	if (reflect.IsReflectProperty()) {
		ReflectProperty(noiseTexture);
		ReflectProperty(noiseParamBuffer);

		ReflectProperty(texCoord)[IShader::BindInput(IShader::BindInput::TEXCOORD)];
		ReflectProperty(noiseOffset)[noiseParamBuffer][IShader::BindInput(IShader::BindInput::GENERAL)];
		ReflectProperty(noiseClip)[noiseParamBuffer][IShader::BindInput(IShader::BindInput::GENERAL)];
	}

	return *this;
}

String MultiHashSetupFS::GetShaderText() {
	return UnifyShaderCode(
		float noise = texture(noiseTexture, texCoord + noiseOffset).x;
		clip(noise - noiseClip);
	);
}