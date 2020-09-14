// SurfaceVS.h
// By PaintDream (paintdream@paintdream.com)
// 2018-4-13
//

#ifndef __SURFACE_VS_H
#define __SURFACE_VS_H

#include "../../../../General/Interface/IShader.h"

namespace PaintsNow {
	class SurfaceVS : public TReflected<SurfaceVS, IShader> {
	public:
		SurfaceVS();
		TObject<IReflect>& operator () (IReflect& reflect) override;
		String GetShaderText() override;
	};
}

#endif // __SURFACE_VS_H
