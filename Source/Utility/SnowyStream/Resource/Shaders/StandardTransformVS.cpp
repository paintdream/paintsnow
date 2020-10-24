#define USE_SWIZZLE
#include "StandardTransformVS.h"
#include "../../../../General/Template/TShaderMacro.h"

using namespace PaintsNow;

StandardTransformVS::StandardTransformVS() : enableInstancing(true), enableSkinning(false), enableViewProjectionMatrix(true), enableVertexColor(false), enableVertexNormal(true), enableInstancedColor(false), enableVertexTangent(true), enableClampedNear(false), enableClampedFar(false) {
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
		ReflectProperty(enableInstancing)[BindConst<bool>()];
		ReflectProperty(enableSkinning)[BindConst<bool>()];
		ReflectProperty(enableVertexNormal)[BindConst<bool>()];
		ReflectProperty(enableVertexColor)[BindConst<bool>()];
		ReflectProperty(enableVertexTangent)[BindConst<bool>()];
		ReflectProperty(enableViewProjectionMatrix)[BindConst<bool>()];
		ReflectProperty(enableInstancedColor)[BindConst<bool>()];
		ReflectProperty(enableClampedNear)[BindConst<bool>()];
		ReflectProperty(enableClampedFar)[BindConst<bool>()];

		ReflectProperty(instanceBuffer)[BindEnable(enableInstancing)];
		ReflectProperty(globalBuffer);
		ReflectProperty(vertexPositionBuffer);
		ReflectProperty(vertexNormalBuffer)[BindEnable(enableVertexNormal)];
		ReflectProperty(vertexTangentBuffer)[BindEnable(enableVertexTangent)];
		ReflectProperty(vertexColorBuffer)[BindEnable(enableVertexColor)];
		ReflectProperty(vertexTexCoordBuffer);

		ReflectProperty(boneIndexBuffer)[BindEnable(enableSkinning)];
		ReflectProperty(boneWeightBuffer)[BindEnable(enableSkinning)];
		ReflectProperty(boneMatricesBuffer)[BindEnable(enableSkinning)];

		static std::vector<float4x4> boneMatries(128); // Just make reflection happy
		ReflectProperty(boneMatries)[BindEnable(enableSkinning)][boneMatricesBuffer][BindInput(BindInput::BONE_TRANSFORMS)];
		ReflectProperty(boneIndex)[BindEnable(enableSkinning)][boneIndexBuffer][BindInput(BindInput::BONE_INDEX)];
		ReflectProperty(boneWeight)[BindEnable(enableSkinning)][boneWeightBuffer][BindInput(BindInput::BONE_WEIGHT)];

		ReflectProperty(worldMatrix)[enableInstancing ? instanceBuffer : globalBuffer][BindInput(BindInput::TRANSFORM_WORLD)];
		ReflectProperty(instancedColor)[enableInstancing ? instanceBuffer : globalBuffer][IShader::BindEnable(enableInstancedColor)][BindInput(BindInput::COLOR_INSTANCED)];
		ReflectProperty(viewMatrix)[globalBuffer][IShader::BindEnable(enableViewProjectionMatrix)][BindInput(BindInput::TRANSFORM_VIEW)];
		ReflectProperty(viewProjectionMatrix)[globalBuffer][IShader::BindEnable(enableViewProjectionMatrix)][BindInput(BindInput::TRANSFORM_VIEWPROJECTION)];

		ReflectProperty(vertexPosition)[vertexPositionBuffer][BindInput(BindInput::POSITION)];
		ReflectProperty(vertexNormal)[vertexNormalBuffer][BindInput(BindInput::NORMAL)];
		ReflectProperty(vertexTangent)[vertexTangentBuffer][BindInput(BindInput::TANGENT)];
		ReflectProperty(vertexColor)[vertexColorBuffer][BindInput(BindInput::COLOR)];
		ReflectProperty(vertexTexCoord)[vertexTexCoordBuffer][BindInput(BindInput::TEXCOORD)];

		ReflectProperty(rasterPosition)[BindOutput(BindOutput::HPOSITION)];
		ReflectProperty(texCoord)[BindOutput(BindOutput::TEXCOORD)];
		ReflectProperty(viewNormal)[BindEnable(enableVertexNormal)][BindOutput(BindOutput::TEXCOORD + 1)];
		ReflectProperty(viewTangent)[BindEnable(enableVertexTangent)][BindOutput(BindOutput::TEXCOORD + 2)];
		ReflectProperty(viewBinormal)[BindEnable(enableVertexTangent)][BindOutput(BindOutput::TEXCOORD + 3)];
		ReflectProperty(tintColor)[BindOutput(BindOutput::COLOR)];
	}

	return *this;
}