// EchoLegend.h -- multi-player client/server support
// By PaintDream (paintdream@paintdream.com)
// 2015-5-29
// This project will be started at .... I don't know.
//

#pragma once
#include "../../Core/Interface/IScript.h"
#include "../../General/Interface/INetwork.h"
#include "../BridgeSunset/BridgeSunset.h"
#include "Connection.h"
#include "Listener.h"
#include "WorkDispatcher.h"

namespace PaintsNow {
	class EchoLegend : public TReflected<EchoLegend, IScript::Library> {
	public:
		EchoLegend(IThread& threadApi, INetwork& network, BridgeSunset& b);
		virtual TObject<IReflect>& operator () (IReflect& reflect) override;
		virtual void ScriptUninitialize(IScript::Request& request);
		TShared<WorkDispatcher> RequestOpenDispatcher(IScript::Request& request);
		void RequestActivateDispatcher(IScript::Request& request, IScript::Delegate<WorkDispatcher> dispatcher);
		void RequestDeactivateDispatcher(IScript::Request& request, IScript::Delegate<WorkDispatcher> dispatcher);
		TShared<Listener> RequestOpenListener(IScript::Request& request, IScript::Delegate<WorkDispatcher> dispatcher, const String& ip, bool http, IScript::Request::Ref eventHandler, IScript::Request::Ref callback, IScript::Request::Ref connectCallback, bool packetMode);
		void RequestActivateListener(IScript::Request& request, IScript::Delegate<Listener> listener);
		String RequestGetListenerAddress(IScript::Request& request, IScript::Delegate<Listener> listener);
		void RequestDeactivateListener(IScript::Request& request, IScript::Delegate<Listener> listener);
		TShared<Connection> RequestOpenConnection(IScript::Request& request, IScript::Delegate<WorkDispatcher> dispatcher, const String& ip, bool http, IScript::Request::Ref connectCallback, bool packetMode);
		void RequestActivateConnection(IScript::Request& request, IScript::Delegate<Connection> listener);
		void RequestDeactivateConnection(IScript::Request& request, IScript::Delegate<Connection> listener);
		void RequestGetConnectionAddresses(IScript::Request& request, IScript::Delegate<Connection> connection);
		void RequestWriteConnection(IScript::Request& request, IScript::Delegate<Connection> connection, const String& data);
		String RequestReadConnection(IScript::Request& request, IScript::Delegate<Connection> connection);
		void RequestWriteConnectionHttpRequest(IScript::Request& request, IScript::Delegate<Connection> connection, const String& uri, const String& method, std::list<std::pair<String, String> >& header, const String& data);
		void RequestWriteConnectionHttpResponse(IScript::Request& request, IScript::Delegate<Connection> connection, int code, const String& data, const String& reason, std::list<std::pair<String, String> >& header);
		void RequestReadConnectionHttpRequest(IScript::Request& request, IScript::Delegate<Connection> connection);
		void RequestParseUri(IScript::Request& request, const String& input);
		String RequestMakeUri(IScript::Request& request, const String& user, const String& host, const String& path, std::list<std::pair<String, String> >& query, const String& fragment);

	protected:
		INetwork& network;
		BridgeSunset& bridgeSunset;
	};
}

