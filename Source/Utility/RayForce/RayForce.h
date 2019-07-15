// RayForce.h
// By PaintDream (paintdream@paintdream.com)
// 2015-6-15
//

#ifndef __RAYFORCE_H__
#define __RAYFORCE_H__

#include "../../Core/Interface/IScript.h"
#include "../SnowyStream/SnowyStream.h"
#include "../BridgeSunset/BridgeSunset.h"
#include "Tunnel.h"
#include "Bridge.h"

namespace PaintsNow {
	namespace NsRayForce {
		class RayForce : public TReflected<RayForce, IScript::Library> {
		public:
			RayForce(IThread& thread, NsSnowyStream::SnowyStream& snowyStream, NsBridgeSunset::BridgeSunset& bridgeSunset);
			virtual TObject<IReflect>& operator () (IReflect& reflect) override;
			void Register(Bridge* bridge);

		public:
			void RequestNewObject(IScript::Request& request, const String& path, const String& data);
			void RequestQueryInterface(IScript::Request& request, IScript::Delegate<Tunnel> tunnel);

		protected:
			NsSnowyStream::SnowyStream& snowyStream;
			NsBridgeSunset::BridgeSunset& bridgeSunset;
			std::list<Bridge*> bridges;
		};
	}
}



#endif // __RAYFORCE_H__