// TextPass.h
// Text Physical Based Shader
// By PaintDream (paintdream@paintdream.com)
//

#ifndef __TEXT_PASS_H__
#define __TEXT_PASS_H__

#include "../../../../General/Misc/PassBase.h"
#include "../Shaders/TextTransformVS.h"
#include "../Shaders/TextShadingFS.h"

namespace PaintsNow {
	namespace NsSnowyStream {
		class TextPass : public TReflected<TextPass, PassBase> {
		public:
			TextPass();

			virtual TObject<IReflect>& operator () (IReflect& reflect) override;

			TextTransformVS textTransform;
			TextShadingFS textShading;
		};
	}
}


#endif // __TEXT_PASS_H__
