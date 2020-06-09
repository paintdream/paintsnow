// Listener.h
// By PaintDream (paintdream@paintdream.com)
// 2015-6-27
//

#ifndef __LISTENER_H__
#define __LISTENER_H__

#include "WorkDispatcher.h"

namespace PaintsNow {
	namespace NsEchoLegend {
		class Listener : public TReflected<Listener, WarpTiny> {
		public:
			Listener(NsBridgeSunset::BridgeSunset& bridgeSunset, INetwork& network, WorkDispatcher* disp, IScript::Request::Ref eventHandler, IScript::Request::Ref callback, IScript::Request::Ref connectCallback, const String& ip, bool http, bool packetMode);
			virtual ~Listener();
			enum {
				LISTENER_HTTP = WARP_CUSTOM_BEGIN,
				LISTENER_PACKET_MODE = WARP_CUSTOM_BEGIN << 1,
				LISTENER_CUSTOM_BEGIN = WARP_CUSTOM_BEGIN << 2
			};

			bool IsValid() const;

			void OnEvent(INetwork::EVENT event);
			const TWrapper<void, INetwork::EVENT> OnAccept(INetwork::Connection* connection);
			void OnAcceptHttp(INetwork::Connection* connection, INetwork::HttpRequest* request);
			virtual void ScriptUninitialize(IScript::Request& request);
			String GetAddress();
			virtual bool Activate();
			virtual void Deactivate();

		protected:
			NsBridgeSunset::BridgeSunset& bridgeSunset;
			INetwork& network;
			TShared<WorkDispatcher> dispatcher;
			INetwork::Listener* listener;
			INetwork::Httpd* httpd;
			IScript::Request::Ref eventHandler;
			IScript::Request::Ref callback;
			IScript::Request::Ref connectCallback;
		};
	}
}

#endif // __LISTENER_H__