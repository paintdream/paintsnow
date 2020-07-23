// ScreenPass.h
// ScreenFS Shader
// By PaintDream (paintdream@paintdream.com)
//

#ifndef __Screen_PASS_H__
#define __Screen_PASS_H__

#include "../../../../General/Misc/PassBase.h"
#include "../Shaders/ScreenTransformVS.h"
#include "../Shaders/ScreenFS.h"
#include "../Shaders/DeferredCompactFS.h"

namespace PaintsNow {
	namespace NsSnowyStream {
		// standard pbr deferred shading Pass using ggx prdf
		class ScreenPass : public TReflected<ScreenPass, PassBase> {
		public:
			ScreenPass();
			virtual TObject<IReflect>& operator () (IReflect& reflect) override;

			// Vertex shaders
			ScreenTransformVS screenTransform;
			// Fragment shaders
			ScreenFS shaderScreen;
		};
	}
}


#endif // __Screen_PASS_H__
