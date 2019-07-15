// WaterPass.h
// Water Physical Based Shader
// By PaintDream (paintdream@paintdream.com)
//

#ifndef __WATER_PASS_H__
#define __WATER_PASS_H__

#include "../../../../General/Misc/ZPassBase.h"

namespace PaintsNow {
	namespace NsSnowyStream {
		class WaterPass : public TReflected<WaterPass, ZPassBase> {
		public:
			WaterPass();
		};
	}
}


#endif // __WATER_PASS_H__
