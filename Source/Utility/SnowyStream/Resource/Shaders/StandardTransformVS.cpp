#define USE_SWIZZLE
#include "StandardTransformVS.h"
#include "../../../../General/Template/TShaderMacro.h"

using namespace PaintsNow;
using namespace PaintsNow::NsSnowyStream;
using namespace PaintsNow::ShaderMacro;

StandardTransformVS::StandardTransformVS() : enableInstancing(true), enableSkinning(false), enableViewProjectionMatrix(true), enableVertexColor(false), enableVertexNormal(true), enableInstancedColor(false), enableVertexTangent(true), enableRasterCoord(false), enableClampedNear(false), enableClampedFar(false) {
	instanceBuffer.description.usage = IRender::Resource::BufferDescription::INSTANCED;
	vertexPositionBuffer.description.usage = IRender::Resource::BufferDescription::VERTEX;
	vertexNormalBuffer.description.usage = IRender::Resource::BufferDescription::VERTEX;
	vertexTangentBuffer.description.usage = IRender::Resource::BufferDescription::VERTEX;
	vertexColorBuffer.description.usage = IRender::Resource::BufferDescription::VERTEX;
	vertexTexCoordBuffer.description.usage = IRender::Resource::BufferDescription::VERTEX;
	globalBuffer.description.usage = IRender::Resource::BufferDescription::UNIFORM;

	boneIndexBuffer.description.usage = IRender::Resource::BufferDescription::VERTEX;
	boneWeightBuffer.description.usage = IRender::Resource::BufferDescription::VERTEX;
	boneMatricesBuffer.description.usage = IRender::Resource::BufferDescription::UNIFORM;
}

String StandardTransformVS::GetShaderText() {
	std::vector<MatrixFloat4x4> boneMatries; // temp fix 
	return UnifyShaderCode(
		float4 position = float4(0, 0, 0, 1);
	position.xyz = vertexPosition;
	if (enableSkinning) {
		// skinning
		float4 sumPosition = float4(0, 0, 0, 0);
		for (int i = 0; i < 4; i++) {
			sumPosition += mult_vec(boneMatries[int(boneIndex[i])], position) * boneWeight[i];
		}

		position.xyz = sumPosition.xyz / sumPosition.w;
	}

	position = mult_vec(worldMatrix, position);

	if (enableViewProjectionMatrix) {
		rasterPosition = mult_vec(viewProjectionMatrix, position);
	} else {
		rasterPosition = position;
	}

	viewNormal = float3(0, 0, 1);
	viewTangent = float3(1, 0, 0);
	viewBinormal = float3(0, 1, 0);
	texCoord = vertexTexCoord;
	if (enableVertexNormal) {
		float4 normal = vertexNormal * float(2.0 / 255.0) - float4(1.0, 1.0, 1.0, 1.0);
		float4x4 viewWorldMatrix = mult_mat(viewMatrix, worldMatrix);
		if (enableVertexTangent) {
			float4 tangent = vertexTangent * float(2.0 / 255.0) - float4(1.0, 1.0, 1.0, 1.0);
			viewTangent = mult_vec(float3x3(viewWorldMatrix), tangent.xyz);
			viewBinormal = mult_vec(float3x3(viewWorldMatrix), normal.xyz);
			viewNormal = cross(viewBinormal, viewTangent);
			viewBinormal *= tangent.w;
		} else {
			// Not precise when non-uniform scaling applied
			viewNormal = mult_vec(float3x3(viewWorldMatrix), normal.xyz);
		}
	}

	tintColor = float4(1, 1, 1, 1);
	if (enableVertexColor) {
		tintColor *= vertexColor;
	}

	if (enableInstancedColor) {
		tintColor *= instancedColor;
	}

	if (enableRasterCoord) {
		rasterCoord = rasterPosition.xy * float(2.0) - float2(1.0, 1.0);
	}

	if (enableClampedFar) {
		rasterPosition.z = max(rasterPosition.z, -1);
	}

	if (enableClampedNear) {
		rasterPosition.z = min(rasterPosition.z, 1);
	});
}

TObject<IReflect>& StandardTransformVS::operator () (IReflect& reflect) {
	BaseClass::operator () (reflect);

	if (reflect.IsReflectProperty()) {
		// options first
		ReflectProperty(enableInstancing)[IShader::BindConst<bool>()];
		ReflectProperty(enableSkinning)[IShader::BindConst<bool>()];
		ReflectProperty(enableVertexNormal)[IShader::BindConst<bool>()];
		ReflectProperty(enableVertexColor)[IShader::BindConst<bool>()];
		ReflectProperty(enableVertexTangent)[IShader::BindConst<bool>()];
		ReflectProperty(enableViewProjectionMatrix)[IShader::BindConst<bool>()];
		ReflectProperty(enableInstancedColor)[IShader::BindConst<bool>()];
		ReflectProperty(enableRasterCoord)[IShader::BindConst<bool>()];
		ReflectProperty(enableClampedNear)[IShader::BindConst<bool>()];
		ReflectProperty(enableClampedFar)[IShader::BindConst<bool>()];

		ReflectProperty(instanceBuffer)[IShader::BindOption(enableInstancing)];
		ReflectProperty(globalBuffer);
		ReflectProperty(vertexPositionBuffer);
		ReflectProperty(vertexNormalBuffer)[IShader::BindOption(enableVertexNormal)];
		ReflectProperty(vertexTangentBuffer)[IShader::BindOption(enableVertexTangent)];
		ReflectProperty(vertexColorBuffer)[IShader::BindOption(enableVertexColor)];
		ReflectProperty(vertexTexCoordBuffer);

		ReflectProperty(boneIndexBuffer)[IShader::BindOption(enableSkinning)];
		ReflectProperty(boneWeightBuffer)[IShader::BindOption(enableSkinning)];
		ReflectProperty(boneMatricesBuffer)[IShader::BindOption(enableSkinning)];

		static std::vector<float4x4> boneMatries(128); // Just make reflection happy
		ReflectProperty(boneMatries)[IShader::BindOption(enableSkinning)][boneMatricesBuffer][IShader::BindInput(IShader::BindInput::BONE_TRANSFORMS)];
		ReflectProperty(boneIndex)[IShader::BindOption(enableSkinning)][boneIndexBuffer][IShader::BindInput(IShader::BindInput::BONE_INDEX)];
		ReflectProperty(boneWeight)[IShader::BindOption(enableSkinning)][boneWeightBuffer][IShader::BindInput(IShader::BindInput::BONE_WEIGHT)];

		ReflectProperty(worldMatrix)[enableInstancing ? instanceBuffer : globalBuffer][IShader::BindInput(IShader::BindInput::TRANSFORM_WORLD)];
		ReflectProperty(instancedColor)[enableInstancing ? instanceBuffer : globalBuffer][IShader::BindOption(enableInstancedColor)][IShader::BindInput(IShader::BindInput::COLOR_INSTANCED)];
		ReflectProperty(viewMatrix)[globalBuffer][IShader::BindOption(enableViewProjectionMatrix)][IShader::BindInput(IShader::BindInput::TRANSFORM_VIEW)];
		ReflectProperty(viewProjectionMatrix)[globalBuffer][IShader::BindOption(enableViewProjectionMatrix)][IShader::BindInput(IShader::BindInput::TRANSFORM_VIEWPROJECTION)];

		ReflectProperty(vertexPosition)[vertexPositionBuffer][IShader::BindInput(IShader::BindInput::POSITION)];
		ReflectProperty(vertexNormal)[vertexNormalBuffer][IShader::BindInput(IShader::BindInput::NORMAL)];
		ReflectProperty(vertexTangent)[vertexTangentBuffer][IShader::BindInput(IShader::BindInput::TANGENT)];
		ReflectProperty(vertexColor)[vertexColorBuffer][IShader::BindInput(IShader::BindInput::COLOR)];
		ReflectProperty(vertexTexCoord)[vertexTexCoordBuffer][IShader::BindInput(IShader::BindInput::TEXCOORD)];

		ReflectProperty(rasterPosition)[IShader::BindOutput(IShader::BindOutput::HPOSITION)];
		ReflectProperty(texCoord)[IShader::BindOutput(IShader::BindOutput::TEXCOORD)];
		ReflectProperty(viewNormal)[IShader::BindOutput(IShader::BindOutput::TEXCOORD + 1)];
		ReflectProperty(viewTangent)[IShader::BindOutput(IShader::BindOutput::TEXCOORD + 2)];
		ReflectProperty(viewBinormal)[IShader::BindOutput(IShader::BindOutput::TEXCOORD + 3)];
		ReflectProperty(rasterCoord)[IShader::BindOption(enableRasterCoord)][IShader::BindOutput(IShader::BindOutput::TEXCOORD + 4)];
		ReflectProperty(tintColor)[IShader::BindOutput(IShader::BindOutput::COLOR)];
	}

	return *this;
}