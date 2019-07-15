// RenderPolicy.h
// PaintDream (paintdream@paintdream.com)
// 2018-9-24
//

#ifndef __RENDERPOLICY_H__
#define __RENDERPOLICY_H__

#include "../../../../Core/System/Tiny.h"

namespace PaintsNow {
	namespace NsMythForest {
		class RenderPolicy : public TReflected<RenderPolicy, SharedTiny> {
		public:
			RenderPolicy();
			virtual TObject<IReflect>& operator () (IReflect& reflect) override;

			String renderPortName;
			uint32_t priority;
		};
	}
}


#endif // __RENDERPOLICY_H__