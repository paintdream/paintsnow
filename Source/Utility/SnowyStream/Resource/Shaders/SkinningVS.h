// SkinningVS.h
// By PaintDream (paintdream@paintdream.com)
// 2018-4-13
//

#ifndef __SKINNING_VS_H
#define __SKINNING_VS_H

#include "../../../../General/Interface/IShader.h"

namespace PaintsNow {
	namespace NsSnowyStream {
		class SkinningVS : public TReflected<SkinningVS, IShader> {
		public:
			SkinningVS();
			virtual TObject<IReflect>& operator () (IReflect& reflect) override;
			virtual String GetShaderText() override;

		protected:
			IShader::BindBuffer boneIndexBuffer;
			IShader::BindBuffer boneWeightBuffer;
			IShader::BindBuffer boneMatricesBuffer;

			static MatrixFloat4x4 _boneMatrix; // Just make reflection happy
			Float4 boneIndex;
			Float4 boneWeight;
			Float3 basePosition;
			Float3 animPosition;
		};
	}
}


#endif // __SKINNING_VS_H
