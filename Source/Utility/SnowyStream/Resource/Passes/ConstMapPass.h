// ConstMapPass.h
// By PaintDream (paintdream@paintdream.com)
//

#ifndef __CONSTMAP_PASS_H__
#define __CONSTMAP_PASS_H__

#include "../../../../General/Misc/ZPassBase.h"
#include "../Shaders/StandardTransformVS.h"
#include "../Shaders/ConstMapFS.h"

namespace PaintsNow {
	namespace NsSnowyStream {
		// standard pbr deferred shading Pass using ggx prdf
		class ConstMapPass : public TReflected<ConstMapPass, ZPassBase> {
		public:
			ConstMapPass();
			virtual TObject<IReflect>& operator () (IReflect& reflect) override;

		protected:
			// Vertex shaders
			StandardTransformVS vertexTransform;
			// Fragment shaders
			ConstMapFS constWriter;
		};
	}
}


#endif // __CONSTMAP_PASS_H__
