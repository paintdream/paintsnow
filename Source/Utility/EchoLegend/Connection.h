// Connection.h
// By PaintDream (paintdream@paintdream.com)
// 2015-6-27
//

#ifndef __CONNECTION_H__
#define __CONNECTION_H__

#include "WorkDispatcher.h"
#include "../../General/Misc/ZMemoryStream.h"

namespace PaintsNow {
	namespace NsEchoLegend {
		class Connection : public TReflected<Connection, WarpTiny> {
		public:
			Connection(NsBridgeSunset::BridgeSunset& bridgeSunset, INetwork& network, WorkDispatcher* dispatcher, IScript::Request::Ref connectCallback, const String& ip, INetwork::Connection* connection, bool http, INetwork::HttpRequest* httpRequest, bool ownReq, bool packetMode);
			enum {
				CONNECTION_HTTP = WARP_CUSTOM_BEGIN,
				CONNECTION_OWN_CONNECTION = WARP_CUSTOM_BEGIN << 1,
				CONNECTION_OWN_REQUEST = WARP_CUSTOM_BEGIN << 2,
				CONNECTION_PACKET_MODE = WARP_CUSTOM_BEGIN << 3,
				CONNECTION_CUSTOM_BEGIN = WARP_CUSTOM_BEGIN << 4
			};

			virtual ~Connection();
			bool IsValid() const;

			virtual bool Activate();
			virtual void Deactivate();

			void OnEvent(INetwork::EVENT event);
			void OnEventHttp(int code);
			virtual void ScriptUninitialize(IScript::Request& request);
			void Read(IScript::Request& request, IScript::Request::Ref callback);
			void Write(IScript::Request& request, const String& data);
			void GetInfo(IScript::Request& request, IScript::Request::Ref callback);
			void ReadHttpRequest(IScript::Request& request, IScript::Request::Ref callback);
			void ReadHttpResponse(IScript::Request& request, IScript::Request::Ref callback);

			void WriteHttpRequest(const String& uri, const String& method, const std::list<std::pair<String, String> >& header, const String& data);
			void WriteHttpResponse(const String& data, int code, const String& reason, std::list<std::pair<String, String> >& header);
			IScript::Request::Ref GetCallback() const;

		protected:
			void DispatchEvent(INetwork::EVENT event);

		protected:
			INetwork& network;
			NsBridgeSunset::BridgeSunset& bridgeSunset;
			WorkDispatcher* dispatcher;
			INetwork::Connection* connection;
			INetwork::HttpRequest* httpRequest;
			IScript::Request::Ref callback;
			String currentData;
			INetwork::Packet currentState;
		};
	}

	IScript::Request& operator >> (IScript::Request& request, std::list<std::pair<String, String> >& mylist);
	IScript::Request& operator << (IScript::Request& request, const std::list<std::pair<String, String> >& mylist);
}

#endif // __CONNECTION_H__
