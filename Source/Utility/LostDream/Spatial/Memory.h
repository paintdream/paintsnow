// Memory.h
// PaintDream (paintdream@paintdream.com)
// 2019-10-11
//

#pragma once
#include "../LostDream.h"

namespace PaintsNow {
	class Memory : public TReflected<Memory, LostDream::Qualifier> {
	public:
		virtual bool Initialize();
		virtual bool Run(int randomSeed, int length);
		virtual void Summary();

		virtual TObject<IReflect>& operator () (IReflect& reflect) override;
	};
}

