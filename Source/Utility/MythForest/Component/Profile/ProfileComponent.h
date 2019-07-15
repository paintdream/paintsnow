// ProfileComponent.h
// By PaintDream (paintdream@paintdream.com)
// 2018-1-19
//

#ifndef __PROFILECOMPONENT_H__
#define __PROFILECOMPONENT_H__

#include "../../Entity.h"
#include "../../Component.h"

namespace PaintsNow {
	namespace NsMythForest {
		class ProfileComponent : public TAllocatedTiny<ProfileComponent, Component> {
		public:
			ProfileComponent(float historyRatio);
			virtual Tiny::FLAG GetEntityFlagMask() const override final;
			virtual void DispatchEvent(Event& event, Entity* entity) override final;
			float GetTickInterval() const;

		protected:
			int64_t timeStamp;
			float tickInterval;
			float historyRatio;
		};
	}
}


#endif // __PROFILECOMPONENT_H__
