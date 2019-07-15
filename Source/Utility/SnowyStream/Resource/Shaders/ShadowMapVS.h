// ShadowMapVS.h
// By PaintDream (paintdream@paintdream.com)
// 2018-4-13
//

#ifndef __SHADOWMAP_VS_H
#define __SHADOWMAP_VS_H

#include "../../../../General/Interface/IShader.h"

namespace PaintsNow {
	namespace NsSnowyStream {
		class ShadowMapVS : public TReflected<ShadowMapVS, IShader> {
		public:
			ShadowMapVS();
			virtual TObject<IReflect>& operator () (IReflect& reflect) override;
		};
	}
}


#endif // __SHADOWMAP_VS_H
