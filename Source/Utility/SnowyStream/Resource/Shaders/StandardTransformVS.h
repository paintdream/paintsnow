// StandardTransformVS.h
// By PaintDream (paintdream@paintdream.com)
// 2018-4-13
//

#ifndef __STANDARDTRANSFORM_VS_H
#define __STANDARDTRANSFORM_VS_H

#include "../../../../General/Interface/IShader.h"

namespace PaintsNow {
	namespace NsSnowyStream {
		class StandardTransformVS : public TReflected<StandardTransformVS, IShader> {
		public:
			StandardTransformVS();
			virtual TObject<IReflect>& operator () (IReflect& reflect) override;
			virtual String GetShaderText() override;

		protected:
			IShader::BindBuffer instanceBuffer;
			IShader::BindBuffer globalBuffer;
			IShader::BindBuffer vertexPositionBuffer;
			IShader::BindBuffer vertexNormalBuffer;
			IShader::BindBuffer vertexTangentBuffer;
			IShader::BindBuffer vertexColorBuffer;
			IShader::BindBuffer vertexTexCoordBuffer;

			IShader::BindBuffer boneIndexBuffer;
			IShader::BindBuffer boneWeightBuffer;
			IShader::BindBuffer boneMatricesBuffer;

			Float4 boneIndex;
			Float4 boneWeight;

			MatrixFloat4x4 worldMatrix;
			MatrixFloat4x4 viewMatrix;
			MatrixFloat4x4 viewProjectionMatrix;

			Float3 vertexPosition;
			float padding;
			Float4 vertexNormal;
			Float4 vertexTangent;
			Float4 vertexColor;
			Float4 vertexTexCoord;
			Float4 instancedColor;
			
			// Output
			Float4 rasterPosition;
			Float2 rasterCoord;
			Float4 texCoord;
			Float3 viewNormal;
			Float3 viewTangent;
			Float3 viewBinormal;
			Float4 tintColor;

		public:
			bool enableInstancing;
			bool enableSkinning;
			bool enableViewProjectionMatrix;
			bool enableVertexNormal;
			bool enableVertexColor;
			bool enableInstancedColor;
			bool enableVertexTangent;
			bool enableRasterCoord;
		};
	}
}


#endif // __STANDARDTRANSFORM_VS_H