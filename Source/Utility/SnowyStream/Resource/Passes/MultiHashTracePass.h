// MultiHashTrace.h
// By PaintDream (paintdream@paintdream.com)
//

#ifndef __MULTIHASHTRACE_PASS_H__
#define __MULTIHASHTRACE_PASS_H__

#include "../../../../General/Misc/PassBase.h"
#include "../Shaders/ScreenTransformVS.h"
#include "../Shaders/MultiHashTraceFS.h"

namespace PaintsNow {
	namespace NsSnowyStream {
		class MultiHashTracePass : public TReflected<MultiHashTracePass, PassBase> {
		public:
			MultiHashTracePass();
			virtual TObject<IReflect>& operator () (IReflect& reflect) override;

			ScreenTransformVS screenTransform;
			MultiHashTraceFS shaderMultiHashTrace;
		};
	}
}


#endif // __MULTIHASHTRACE_PASS_H__
