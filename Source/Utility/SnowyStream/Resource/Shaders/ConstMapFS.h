// ConstMapFS.h
// By PaintDream (paintdream@paintdream.com)
// 2018-4-13
//

#ifndef __CONSTMAP_FS_H
#define __CONSTMAP_FS_H

#include "../../../../General/Interface/IShader.h"

namespace PaintsNow {
	namespace NsSnowyStream {
		class ConstMapFS : public TReflected<ConstMapFS, IShader> {
		public:
			ConstMapFS();
			virtual TObject<IReflect>& operator () (IReflect& reflect) override;
			virtual String GetShaderText() override;

		public:
			IShader::BindBuffer valueBuffer;

		protected:
			// varyings
			Float4 tintColor;

			// targets
			Float4 target;
		};
	}
}


#endif // __CONSTMAP_FS_H