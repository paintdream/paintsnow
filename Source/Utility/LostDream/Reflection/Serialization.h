// Serialization.h
// By PaintDream (paintdream@paintdream.com)
// 2018-2-10
//

#pragma once
#include "../LostDream.h"

namespace PaintsNow {
	class Serialization : public TReflected<Serialization, LostDream::Qualifier> {
	public:
		virtual bool Initialize();
		virtual bool Run(int randomSeed, int length);
		virtual void Summary();

		virtual TObject<IReflect>& operator () (IReflect& reflect) override;
	};
}

