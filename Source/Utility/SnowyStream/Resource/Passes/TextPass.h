// TextPass.h
// Text Physical Based Shader
// By PaintDream (paintdream@paintdream.com)
//

#pragma once
#include "../../../../General/Misc/PassBase.h"
#include "../Shaders/TextTransformVS.h"
#include "../Shaders/TextShadingFS.h"

namespace PaintsNow {
	class TextPass : public TReflected<TextPass, PassBase> {
	public:
		TextPass();

		virtual TObject<IReflect>& operator () (IReflect& reflect) override;

		TextTransformVS textTransform;
		TextShadingFS textShading;
	};
}
