// MultiHashSetup.h
// By PaintDream (paintdream@paintdream.com)
//

#ifndef __MULTIHASHSETUP_PASS_H__
#define __MULTIHASHSETUP_PASS_H__

#include "../../../../General/Misc/PassBase.h"
#include "../Shaders/StandardTransformVS.h"
#include "../Shaders/StandardParameterFS.h"
#include "../Shaders/DeferredCompactFS.h"
#include "../Shaders/MultiHashSetupFS.h"

namespace PaintsNow {
	namespace NsSnowyStream {
		class MultiHashSetupPass : public TReflected<MultiHashSetupPass, PassBase> {
		public:
			MultiHashSetupPass();
			virtual TObject<IReflect>& operator () (IReflect& reflect) override;

		protected:
			// Vertex shaders
			StandardTransformVS standardTransform;
			// Fragment shaders
			MultiHashSetupFS shaderMultiHashSetup;
			StandardParameterFS shaderParameter;
			DeferredCompactEncodeFS shaderCompactEncode;
		};
	}
}


#endif // __MULTIHASHSETUP_PASS_H__
