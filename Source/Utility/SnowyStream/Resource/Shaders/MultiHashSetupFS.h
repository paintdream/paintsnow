// MultiHashSetupFS.h
// By PaintDream (paintdream@paintdream.com)
// 2018-4-13
//

#ifndef __MULTIHASHSETUP_FS_H
#define __MULTIHASHSETUP_FS_H

#include "../../../../General/Interface/IShader.h"

namespace PaintsNow {
	class MultiHashSetupFS : public TReflected<MultiHashSetupFS, IShader> {
	public:
		MultiHashSetupFS();
		TObject<IReflect>& operator () (IReflect& reflect) override;
		String GetShaderText() override;

		BindTexture noiseTexture;

		Float2 rasterCoord;
		Float4 tintColor;
	};
}

#endif // __MULTIHASHSETUP_FS_H
