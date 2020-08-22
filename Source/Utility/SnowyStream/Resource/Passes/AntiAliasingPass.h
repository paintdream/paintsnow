// AntiAliasingPass.h
// AntiAliasingFS Pass
// By PaintDream (paintdream@paintdream.com)
//

#pragma once
#include "../../../../General/Misc/PassBase.h"
#include "../Shaders/ScreenTransformVS.h"
#include "../Shaders/AntiAliasingFS.h"

namespace PaintsNow {
	class AntiAliasingPass : public TReflected<AntiAliasingPass, PassBase> {
	public:
		AntiAliasingPass();
		virtual TObject<IReflect>& operator () (IReflect& reflect) override;

	public:
		// Vertex shaders
		ScreenTransformVS transform;

		// Fragment shaders
		AntiAliasingFS antiAliasing;
	};
}
