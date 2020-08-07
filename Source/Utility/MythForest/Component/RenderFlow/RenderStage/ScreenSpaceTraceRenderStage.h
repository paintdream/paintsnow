// ScreenSpaceTraceRenderStage.h
// PaintDream (paintdream@paintdream.com)
// 2018-9-11
//

#ifndef __SCREENSPACETRACERENDERSTAGE_H__
#define __SCREENSPACETRACERENDERSTAGE_H__

#include "../RenderStage.h"
#include "../RenderPort/RenderPortTextureInput.h"

namespace PaintsNow {
	namespace NsMythForest {
		class ScreenSpaceTraceRenderStage : public TReflected<ScreenSpaceTraceRenderStage, RenderStage> {
		public:
			ScreenSpaceTraceRenderStage(const String& s);

			RenderPortTextureInput Depth;
		};
	}
}

#endif // __SCREENSPACETRACERENDERSTAGE_H__