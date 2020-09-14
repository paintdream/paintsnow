// GalaxyWeaver
// By PaintDream
// 

#pragma once
#include "../../Core/Interface/IScript.h"
#include "../../General/Interface/ITunnel.h"
#include "Weaver.h"
#include "../MythForest/Entity.h"

namespace PaintsNow {
	class GalaxyWeaver : public TReflected<GalaxyWeaver, IScript::Library> {
	public:
		GalaxyWeaver(IThread& threadApi, ITunnel& network, BridgeSunset& bridgeSunset, SnowyStream& snowyStream, MythForest& mythForest);
		TObject<IReflect>& operator () (IReflect& reflect) override;

	public:
		TShared<Weaver> RequestNewWeaver(IScript::Request& request, const String& config);
		void RequestConnectEntity(IScript::Request& requst, IScript::Delegate<Weaver> weaver, IScript::Delegate<Entity> scene);
		void RequestSetWeaverRpcCallback(IScript::Request& request, IScript::Delegate<Weaver> weaver, IScript::Request::Ref callback);
		void RequestSetWeaverConnectionCallback(IScript::Request& request, IScript::Delegate<Weaver> weaver, IScript::Request::Ref callback);
		void RequestStartWeaver(IScript::Request& request, IScript::Delegate<Weaver> weaver);
		void RequestStopWeaver(IScript::Request& request, IScript::Delegate<Weaver> weaver);
		void RequestCommitWeaverChanges(IScript::Request& request, IScript::Delegate<Weaver> weaver);

	protected:
		ITunnel& network;
		BridgeSunset& bridgeSunset;
		SnowyStream& snowyStream;
		MythForest& mythForest;
	};
}

