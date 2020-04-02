// MultiHashSetupFS.h
// By PaintDream (paintdream@paintdream.com)
// 2018-4-13
//

#ifndef __MULTIHASHSETUP_FS_H
#define __MULTIHASHSETUP_FS_H

#include "../../../../General/Interface/IShader.h"

namespace PaintsNow {
	namespace NsSnowyStream {
		class MultiHashSetupFS : public TReflected<MultiHashSetupFS, IShader> {
		public:
			MultiHashSetupFS();
			virtual TObject<IReflect>& operator () (IReflect& reflect) override;
			virtual String GetShaderText() override;
			
			BindTexture noiseTexture;
			BindBuffer noiseParamBuffer;

			Float2 rasterCoord;
			Float2 noiseOffset;
			float noiseClip;
		};
	}
}


#endif // __MULTIHASHSETUP_FS_H
