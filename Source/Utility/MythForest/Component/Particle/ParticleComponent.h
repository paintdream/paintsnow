// ParticleComponent.h
// By PaintDream (paintdream@paintdream.com)
// 2015-5-5
//

#ifndef __PARTICLECOMPONENT_H__
#define __PARTICLECOMPONENT_H__

#include "../../Component.h"
#include "../../../SnowyStream/SnowyStream.h"

namespace PaintsNow {
	namespace NsMythForest {
		class ParticleComponent : public TAllocatedTiny<ParticleComponent, Component> {
		public:
			ParticleComponent();
			virtual ~ParticleComponent();
		};
	}
}


#endif // __PARTICLECOMPONENT_H__