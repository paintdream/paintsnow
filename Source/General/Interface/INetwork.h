// INetwork.h
// By PaintDream (paintdream@paintdream.com)
// 2015-1-25
//

#pragma once
#include "../../Core/PaintsNow.h"
#include "ITunnel.h"

namespace PaintsNow {
	class INetwork : public ITunnel {
	public:
		virtual ~INetwork();
		class Httpd {};
		class HttpRequest {};

		virtual Dispatcher* OpenDispatcher() = 0;
		virtual bool ActivateDispatcher(Dispatcher* dispatcher) = 0;
		virtual void DeactivateDispatcher(Dispatcher* dispatcher) = 0;
		virtual Dispatcher* GetListenerDispatcher(Listener* listener) = 0;
		virtual Dispatcher* GetConnectionDispatcher(Connection* connection) = 0;
		virtual void CloseDispatcher(Dispatcher* dispatcher) = 0;

		virtual void EnumerateIPAddresses(const TWrapper<void, const String&>& callback) = 0;

		// derived from ITunnel
		virtual Listener* OpenListener(Dispatcher* dispatcher, const TWrapper<void, EVENT>& eventHandler, const TWrapper<const TWrapper<void, EVENT>, Connection*>& callback, const String& address) = 0;
		virtual bool ActivateListener(Listener* listener) = 0;
		virtual void GetListenerInfo(Listener* listener, String& address) = 0;
		virtual void DeactivateListener(Listener* listener) = 0;
		virtual void CloseListener(Listener* listener) = 0;

		virtual Connection* OpenConnection(Dispatcher* dispatcher, const TWrapper<void, EVENT>& callback, const String& address) = 0;
		virtual bool ActivateConnection(Connection* connection) = 0;
		virtual void GetConnectionInfo(Connection* connection, String& from, String& to) = 0;
		virtual void Flush(Connection* connection) = 0;
		virtual bool ReadConnection(Connection* connection, void* data, size_t& length) = 0;
		virtual bool WriteConnection(Connection* connection, const void* data, size_t& length) = 0;
		virtual void DeactivateConnection(Connection* connection) = 0;
		virtual void CloseConnection(Connection* connection) = 0;

		virtual Httpd* OpenHttpd(Listener* listener, const TWrapper<void, Connection*, HttpRequest*>& requestHandler) = 0;
		virtual bool ActivateHttpd(Httpd* httpd) = 0;
		virtual void CloseHttpd(Httpd* httpd) = 0;

		// URI, METHOD, CONTENT
		virtual void PrepareHttpRequest(HttpRequest* request) = 0;
		virtual String GetHttpRequestUri(HttpRequest* request) = 0;
		virtual void SetHttpRequestUri(HttpRequest* request, const String& uri) = 0;
		virtual String GetHttpRequestMethod(HttpRequest* request) = 0;
		virtual void SetHttpRequestMethod(HttpRequest* request, const String& method) = 0;
		virtual void GetHttpRequestHeader(HttpRequest* request, std::list<std::pair<String, String> >& header) = 0;
		virtual String GetHttpRequestData(HttpRequest* request) = 0;
		virtual void SetHttpRequestData(HttpRequest* request, const String& uri) = 0;
		virtual void SetHttpRequestHeader(HttpRequest* request, const std::list<std::pair<String, String> >& header) = 0;
		virtual void ParseUri(const String& uri, String& user, String& host, int& port, String& path, std::list<std::pair<String, String> >& query, String& fragment) = 0;
		virtual String MakeUri(const String& user, const String& host, const String& path, const std::list<std::pair<String, String> >& query, const String& fragment) = 0;

		virtual void MakeHttpRequest(HttpRequest* request) = 0;
		virtual void MakeHttpResponse(HttpRequest* request, int code, const String& reason, const String& data) = 0;

		virtual HttpRequest* OpenHttpRequest(Connection* connection, const TWrapper<void, int>& callback) = 0;
		virtual void CloseHttpRequest(HttpRequest* request) = 0;
	};
}

