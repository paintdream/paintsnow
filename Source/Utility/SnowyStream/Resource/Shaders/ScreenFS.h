// ScreenFS.h
// By PaintDream (paintdream@paintdream.com)
// 2018-4-13
//

#ifndef __SCREEN_FS_H
#define __SCREEN_FS_H

#include "../../../../General/Interface/IShader.h"

namespace PaintsNow {
	namespace NsSnowyStream {
		class ScreenFS : public TReflected<ScreenFS, IShader> {
		public:
			ScreenFS();
			virtual TObject<IReflect>& operator () (IReflect& reflect) override;
			virtual String GetShaderText() override;

		public:
			BindTexture inputColorTexture;
			BindTexture inputBloomTexture0;
			BindTexture inputBloomTexture1;
			BindTexture inputBloomTexture2;

		protected:
			Float2 rasterCoord;
			Float4 outputColor;

			float bloomIntensity;
		};
	}
}


#endif // __SCREEN_FS_H
