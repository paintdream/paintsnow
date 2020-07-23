// TerrainShaderResource.h
// Terrain Physical Based Shader
// By PaintDream (paintdream@paintdream.com)
//

#ifndef __TERRAIN_PASS_H__
#define __TERRAIN_PASS_H__

#include "../../../../General/Misc/PassBase.h"

namespace PaintsNow {
	namespace NsSnowyStream {
		class TerrainPass : public TReflected<TerrainPass, PassBase> {
		public:
			TerrainPass();
		};
	}
}


#endif // __TERRAIN_PASS_H__
