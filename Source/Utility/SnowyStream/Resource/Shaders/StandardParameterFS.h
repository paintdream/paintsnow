// StandardParameterFS.h
// By PaintDream (paintdream@paintdream.com)
// 2018-4-13
//

#ifndef __STANDARDPARAMETER_FS_H
#define __STANDARDPARAMETER_FS_H

#include "../../../../General/Interface/IShader.h"

namespace PaintsNow {
	namespace NsSnowyStream {
		class StandardParameterFS : public TReflected<StandardParameterFS, IShader> {
		public:
			StandardParameterFS();
			virtual TObject<IReflect>& operator () (IReflect& reflect) override;

			virtual String GetShaderText() override;

		protected:
			BindTexture baseColorTexture;
			BindTexture normalTexture;
			BindTexture mixtureTexture;

			/*
			BindBuffer paramBuffer;
			Float2 invScreenSize;
			float timestamp;
			float reserved;*/

			Float4 texCoord;
			Float3 viewNormal;
			Float3 viewTangent;
			Float3 viewBinormal;

			Float4 tintColor;
			Float3 outputColor;
			Float3 outputNormal;
			float alpha;
			float metallic;
			float roughness;
			float occlusion;

			bool enableBaseColorTexture;
			bool enableNormalTexture;
			bool enableMaterialTexture;
		};
	}
}


#endif // __STANDARDPARAMETER_FS_H
