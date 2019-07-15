// VolumeShaderResource.h
// Volume Physical Based Shader
// By PaintDream (paintdream@paintdream.com)
//

#ifndef __VOLUME_PASS_H__
#define __VOLUME_PASS_H__

#include "../../../../General/Misc/ZPassBase.h"

namespace PaintsNow {
	namespace NsSnowyStream {
		class VolumePass : public TReflected<VolumePass, ZPassBase> {
		public:
			VolumePass();
		};
	}
}


#endif // __VOLUME_PASS_H__
