// WidgetTransformVS.h
// By PaintDream (paintdream@paintdream.com)
// 2018-4-13
//

#ifndef __WIDGETTRANSFORM_VS_H
#define __WIDGETTRANSFORM_VS_H

#include "../../../../General/Interface/IShader.h"

namespace PaintsNow {
	namespace NsSnowyStream {
		class WidgetTransformVS : public TReflected<WidgetTransformVS, IShader> {
		public:
			WidgetTransformVS();
			virtual TObject<IReflect>& operator () (IReflect& reflect) override;

			virtual String GetShaderText() override;

		public:
			IShader::BindBuffer instanceBuffer;
			IShader::BindBuffer vertexBuffer;

			// Input attribs
			Float3 unitPositionTexCoord;
			float _padding;

			// instanced
			Float4 inputPositionRect; // instanced ..
			Float4 inputTexCoordRect;
			Float4 inputTexCoordMark;
			Float4 inputTexCoordScale;
			Float4 inputTintColor;

		protected:
			// Output vars
			Float4 position;

			// varyings
			Float4 texCoord;
			Float4 texCoordRect;
			Float4 texCoordMark;
			Float4 texCoordScale;
			Float4 tintColor;
		};
	}
}


#endif // __WIDGETTRANSFORM_VS_H