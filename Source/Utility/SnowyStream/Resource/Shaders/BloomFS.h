// BloomFS.h
// By PaintDream (paintdream@paintdream.com)
// 2018-4-13
//

#ifndef __Bloom_FS_H
#define __Bloom_FS_H

#include "../../../../General/Interface/IShader.h"

namespace PaintsNow {
	namespace NsSnowyStream {
		class BloomFS : public TReflected<BloomFS, IShader> {
		public:
			BloomFS();
			virtual TObject<IReflect>& operator () (IReflect& reflect) override;
			virtual String GetShaderText() override;

			BindTexture screenTexture;
			BindBuffer uniformBloomBuffer;
			Float2 invScreenSize;

		protected:
			Float2 rasterCoord;
			Float4 outputColor;
		};
	}
}


#endif // __Bloom_FS_H
