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

			// instanced
			MatrixFloat4x4 worldMatrix;
			Float4 subTexMark;
			Float4 mainCoordRect;

		protected:
			// Output vars
			Float4 position;

			// varyings
			Float4 texCoord;
			Float4 texCoordRect;
			Float4 texCoordMark;
			Float4 texCoordScale;
		};
	}
}


#endif // __WIDGETTRANSFORM_VS_H