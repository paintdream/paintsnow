// GalaxyWeaver
// By PaintDream
// 

#ifndef __GALAXYWEAVER_H__
#define __GALAXYWEAVER_H__

#include "../../Core/Interface/IScript.h"
#include "../../General/Interface/ITunnel.h"
#include "Weaver.h"
#include "../MythForest/Entity.h"

namespace PaintsNow {
	namespace NsGalaxyWeaver {
		class GalaxyWeaver : public TReflected<GalaxyWeaver, IScript::Library> {
		public:
			GalaxyWeaver(IThread& threadApi, ITunnel& network, NsBridgeSunset::BridgeSunset& bridgeSunset, NsSnowyStream::SnowyStream& snowyStream, NsMythForest::MythForest& mythForest);
			virtual TObject<IReflect>& operator () (IReflect& reflect) override;

		public:
			TShared<Weaver> RequestNewWeaver(IScript::Request& request, const String& config);
			void RequestConnectEntity(IScript::Request& requst, IScript::Delegate<Weaver> weaver, IScript::Delegate<NsMythForest::Entity> scene);
			void RequestSetWeaverRpcCallback(IScript::Request& request, IScript::Delegate<Weaver> weaver, IScript::Request::Ref callback);
			void RequestSetWeaverConnectionCallback(IScript::Request& request, IScript::Delegate<Weaver> weaver, IScript::Request::Ref callback);
			void RequestStartWeaver(IScript::Request& request, IScript::Delegate<Weaver> weaver);
			void RequestStopWeaver(IScript::Request& request, IScript::Delegate<Weaver> weaver);
			void RequestCommitWeaverChanges(IScript::Request& request, IScript::Delegate<Weaver> weaver);

		protected:
			ITunnel& network;
			NsBridgeSunset::BridgeSunset& bridgeSunset;
			NsSnowyStream::SnowyStream& snowyStream;
			NsMythForest::MythForest& mythForest;
		};
	}
}


#endif // __GALAXYWEAVER_H__