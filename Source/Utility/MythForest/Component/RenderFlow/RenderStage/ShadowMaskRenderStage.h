// ShadowMaskRenderStage.h
// PaintDream (paintdream@paintdream.com)
// 2018-9-11
//

#ifndef __SHADOWMASKRENDERSTAGE_H__
#define __SHADOWMASKRENDERSTAGE_H__

#include "../RenderStage.h"

namespace PaintsNow {
	namespace NsMythForest {
		class ShadowMaskRenderStage : public TReflected<ShadowMaskRenderStage, RenderStage> {
		public:
			ShadowMaskRenderStage();
		};
	}
}

#endif // __SHADOWMASKRENDERSTAGE_H__