// RemoteComponent.h
// By PaintDream (paintdream@paintdream.com)
// 2015-5-5
//

#ifndef __REMOTECOMPONENT_H__
#define __REMOTECOMPONENT_H__

#include "../../Component.h"
#include "../../../SnowyStream/SnowyStream.h"

namespace PaintsNow {
	namespace NsMythForest {
		class RemoteComponent : public TAllocatedTiny<RemoteComponent, Component> {
		public:
			RemoteComponent();
			virtual ~RemoteComponent();
		};
	}
}


#endif // __REMOTECOMPONENT_H__