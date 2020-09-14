// LightBufferPass.h
// LightBufferFS Pass
// By PaintDream (paintdream@paintdream.com)
//

#pragma once
#include "../../../../General/Misc/PassBase.h"
#include "../Shaders/ScreenTransformVS.h"
#include "../Shaders/LightEncoderFS.h"

namespace PaintsNow {
	class LightBufferPass : public TReflected<LightBufferPass, PassBase> {
	public:
		LightBufferPass();
		TObject<IReflect>& operator () (IReflect& reflect) override;

	public:
		// Vertex shaders
		ScreenTransformVS transform;

		// Fragment shaders
		LightEncoderFS encoder;
	};
}
