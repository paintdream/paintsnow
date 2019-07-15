// DeferredCompact.h
// By PaintDream (paintdream@paintdream.com)
// 2018-4-13
//

#ifndef __TILEBASEDLIGHT_CS_H
#define __TILEBASEDLIGHT_CS_H

#include "../../../../General/Interface/IShader.h"

namespace PaintsNow {
	namespace NsSnowyStream {
		class TileBasedLightCS : public TReflected<TileBasedLightCS, IShader> {
		public:
			TileBasedLightCS();
			virtual TObject<IReflect>& operator () (IReflect& reflect) override;
			virtual String GetShaderText() override;
		};
	}
}


#endif // __TILEBASEDLIGHT_CS_H
