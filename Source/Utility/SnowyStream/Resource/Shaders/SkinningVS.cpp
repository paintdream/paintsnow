#define USE_SWIZZLE
#include "SkinningVS.h"
#include "../../../../General/Template/TShaderMacro.h"

using namespace PaintsNow;
using namespace PaintsNow::NsSnowyStream;
using namespace PaintsNow::ShaderMacro;

namespace PaintsNow {
	namespace NsSnowyStream {
		MatrixFloat4x4 SkinningVS::_boneMatrix;
	}
}

SkinningVS::SkinningVS() {
	boneIndexBuffer.description.usage = IRender::Resource::BufferDescription::VERTEX;
	boneWeightBuffer.description.usage = IRender::Resource::BufferDescription::VERTEX;
	boneMatricesBuffer.description.usage = IRender::Resource::BufferDescription::UNIFORM;
}

String SkinningVS::GetShaderText() {
	std::vector<MatrixFloat4x4> boneMatricesBuffer; // temp fix 
	return UnifyShaderCode(
		Float4 sumPosition = Float4(0, 0, 0, 0);
		Float4 localPosition = Float4(0, 0, 0, 1);
		localPosition.xyz = basePosition.xyz;
		// skinning
		for (int i = 0; i < 4; i++) {
			sumPosition += mult_vec(boneMatricesBuffer[int(boneIndex[i])], localPosition) * boneWeight[i];
		}
		animPosition = sumPosition.xyz / sumPosition.w;
	);
}

TObject<IReflect>& SkinningVS::operator () (IReflect& reflect) {
	BaseClass::operator () (reflect);

	if (reflect.IsReflectProperty()) {
		ReflectProperty(boneIndexBuffer);
		ReflectProperty(boneWeightBuffer);
		ReflectProperty(boneMatricesBuffer);

		ReflectProperty(_boneMatrix)[boneMatricesBuffer][IShader::BindInput(IShader::BindInput::BONE_TRANSFORMS)];
		ReflectProperty(boneIndex)[boneIndexBuffer][IShader::BindInput(IShader::BindInput::BONE_INDEX)];
		ReflectProperty(boneWeight)[boneWeightBuffer][IShader::BindInput(IShader::BindInput::BONE_WEIGHT)];
		ReflectProperty(basePosition)[IShader::BindInput(IShader::BindInput::LOCAL)];
		ReflectProperty(animPosition)[IShader::BindOutput(IShader::BindOutput::LOCAL)];
	}

	return *this;
}
