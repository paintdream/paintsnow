// ZNetworkLibEvent.h
// By PaintDream (paintdream@paintdream.com)
// 2015-10-24
//

#pragma once
#include "../../../Interface/INetwork.h"
#include "../../../../Core/Interface/IThread.h"

namespace PaintsNow {
	class ZNetworkLibEvent final : public INetwork {
	public:
		ZNetworkLibEvent(IThread& threadApi);
		virtual ~ZNetworkLibEvent();
		virtual void EnumerateIPAddresses(const TWrapper<void, const String&>& callback);
		virtual Dispatcher* OpenDispatcher();
		virtual bool ActivateDispatcher(Dispatcher* dispatcher);
		virtual void DeactivateDispatcher(Dispatcher* dispatcher);
		virtual Dispatcher* GetListenerDispatcher(Listener* listener);
		virtual Dispatcher* GetConnectionDispatcher(Connection* connection);
		virtual void CloseDispatcher(Dispatcher* dispatcher);

		virtual Listener* OpenListener(Dispatcher* dispatcher, const TWrapper<void, EVENT>& eventHandler, const TWrapper<const TWrapper<void, EVENT>, Connection*>& callback, const String& ip);
		virtual bool ActivateListener(Listener* listener);
		virtual void GetListenerInfo(Listener* listener, String& ip);
		virtual void DeactivateListener(Listener* listener);
		virtual void CloseListener(Listener* listener);

		virtual Connection* OpenConnection(Dispatcher* dispatcher, const TWrapper<void, EVENT>& connectCallback, const String& ip);
		virtual bool ActivateConnection(Connection* connection);
		virtual void DeactivateConnection(Connection* connection);
		virtual bool ReadConnection(Connection* connection, void* data, size_t& length);
		virtual bool WriteConnection(Connection* connection, const void* data, size_t& length);
		virtual void Flush(Connection* connection);
		virtual void GetConnectionInfo(Connection* connection, String& src, String& dst);
		virtual void CloseConnection(Connection* connection);

		virtual Httpd* OpenHttpd(Listener* listener, const TWrapper<void, Connection*, HttpRequest*>& requestHandler);
		virtual bool ActivateHttpd(Httpd* httpd);
		virtual void CloseHttpd(Httpd* httpd);

		// URI, METHOD, CONTENT
		virtual void PrepareHttpRequest(HttpRequest* request);
		virtual String GetHttpRequestUri(HttpRequest* request);
		virtual void SetHttpRequestUri(HttpRequest* request, const String& uri);
		virtual String GetHttpRequestMethod(HttpRequest* request);
		virtual void SetHttpRequestMethod(HttpRequest* request, const String& method);
		virtual void GetHttpRequestHeader(HttpRequest* request,  std::list<std::pair<String, String> >& header);
		virtual void SetHttpRequestHeader(HttpRequest* request, const std::list<std::pair<String, String> >& header);

		virtual String GetHttpRequestData(HttpRequest* request);
		virtual void SetHttpRequestData(HttpRequest* request, const String& data);
		virtual void ParseUri(const String& uri, String& user, String& host, int& port, String& path, std::list<std::pair<String, String> >& query, String& fragment);
		virtual String MakeUri(const String& user, const String& host, const String& path, const std::list<std::pair<String, String> >& query, const String& fragment);

		virtual void MakeHttpRequest(HttpRequest* request);
		virtual void MakeHttpResponse(HttpRequest* request, int code, const String& reason, const String& data);

		virtual HttpRequest* OpenHttpRequest(Connection* connection, const TWrapper<void, int>& callback);
		virtual void CloseHttpRequest(HttpRequest* request);

	protected:
		virtual bool ActivateListenerWithHttpd(Listener* listener, Httpd* httpd);

	protected:
		IThread& threadApi;
	};
}
