#include "Connection.h"

using namespace PaintsNow;

Connection::Connection(BridgeSunset& bs, INetwork& nt, WorkDispatcher* disp, IScript::Request::Ref cb, const String& ip, INetwork::Connection* con, bool http, INetwork::HttpRequest* httpReq, bool ownReq, bool mode) : bridgeSunset(bs), network(nt),  dispatcher(disp), connection(con), httpRequest(httpReq), callback(cb) {
	uint32_t bitMask = 0;
	if (ownReq) {
		bitMask |= CONNECTION_OWN_REQUEST;
	}

	if (mode) {
		bitMask |= CONNECTION_PACKET_MODE;
	}

	if (http) {
		bitMask |= CONNECTION_HTTP;
	}

	if (connection == nullptr) {
		connection = network.OpenConnection(dispatcher->GetDispatcher(), Wrap(this, &Connection::OnEvent), ip);
		bitMask |= CONNECTION_OWN_CONNECTION;
	} else {
		bitMask |= TINY_ACTIVATED;
	}

	Flag().fetch_or(bitMask, std::memory_order_relaxed);

	if (connection != nullptr) {
		dispatcher->ReferenceObject();

		if (Flag().load(std::memory_order_relaxed) & CONNECTION_HTTP) {
			if (httpRequest == nullptr) {
				httpRequest = network.OpenHttpRequest(connection, Wrap(this, &Connection::OnEventHttp));
				Flag().fetch_or(CONNECTION_OWN_REQUEST, std::memory_order_relaxed);
			}
		}
	}
}

bool Connection::IsValid() const {
	return connection != nullptr && (!(Flag().load(std::memory_order_acquire) & CONNECTION_HTTP) || httpRequest != nullptr);
}

void Connection::Deactivate() {
	if (Flag().fetch_and(~TINY_ACTIVATED, std::memory_order_relaxed) & TINY_ACTIVATED) {
		network.DeactivateConnection(connection);
	}
}

Connection::~Connection() {
	if (httpRequest != nullptr && (Flag().load(std::memory_order_acquire) & CONNECTION_OWN_REQUEST)) {
		network.CloseHttpRequest(httpRequest);
	}

	if (connection != nullptr && (Flag().load(std::memory_order_acquire) & CONNECTION_OWN_CONNECTION)) {
		network.CloseConnection(connection);
	}

	if (dispatcher != nullptr) {
		dispatcher->ReleaseObject();
	}
}

bool Connection::Activate() {
	bool ret = false;
	if (!(Flag().fetch_or(TINY_ACTIVATED, std::memory_order_acquire) & TINY_ACTIVATED)) {
		ret = network.ActivateConnection(connection);
	}

	return ret;
}

IScript::Request::Ref Connection::GetCallback() const {
	return callback;
}

void Connection::OnEventHttp(int code) {
	IScript::Request& req = *bridgeSunset.AcquireSafe();
	req.DoLock();
	req.Push();
	req.Call(sync, callback, this, code);
	req.Pop();
	req.UnLock();
	bridgeSunset.ReleaseSafe(&req);
//	bridgeSunset.GetKernel().QueueRoutine(this, CreateTaskScript(callback, this, code));
}

struct Header {
	uint32_t packetLength;
	uint32_t packetHash;
};

void Connection::OnEvent(INetwork::EVENT event) {
	if (event == INetwork::READ && (Flag().load(std::memory_order_acquire) & CONNECTION_PACKET_MODE)) {
		String segment;
		INetwork::PacketSizeType blockSize = 0x1000;
		segment.resize(blockSize);
		while (network.ReadConnectionPacket(connection, const_cast<char*>(segment.data()), blockSize, currentState)) {
			// new packet?
			currentData.append(segment.data(), blockSize);
			if (currentState.cursor == currentState.header.length) {
				DispatchEvent(event);
				currentData.resize(0);
			}
		}
	} else {
		DispatchEvent(event);

		if ((Flag().load(std::memory_order_acquire) & CONNECTION_HTTP) && event == INetwork::CONNECTED) {
			OnEvent(INetwork::READ);
		}
	}
}

void Connection::DispatchEvent(INetwork::EVENT event) {
	IScript::Request& req = *bridgeSunset.AcquireSafe();
	req.DoLock();
	req.Push();
	if ((Flag().load(std::memory_order_relaxed) & CONNECTION_PACKET_MODE)) {
		req << currentData;
	}
	req.Call(sync, callback, this, Looper::EventToString(event));
	req.Pop();
	req.UnLock();
	bridgeSunset.ReleaseSafe(&req);
}

void Connection::ScriptUninitialize(IScript::Request& request) {
	if (callback) {
		request.DoLock();
		request.Dereference(callback);
		request.UnLock();
	}

	SharedTiny::ScriptUninitialize(request);
}

String Connection::Read() {
	if (!(Flag().load(std::memory_order_acquire) & TINY_ACTIVATED)) {
		assert(!(Flag().load(std::memory_order_acquire) & CONNECTION_PACKET_MODE));

		size_t length = 0;
		network.ReadConnection(connection, nullptr, length);
		String data;
		data.resize(length);
		network.ReadConnection(connection, const_cast<char*>(data.data()), length);

		return data;
	} else {
		return "";
	}
}

void Connection::Write(const String& data) {
	if (!(Flag().load(std::memory_order_acquire) & TINY_ACTIVATED)) {
		if (Flag().load(std::memory_order_relaxed) & CONNECTION_PACKET_MODE) {
			INetwork::Packet packet;
			INetwork::PacketSizeType length = (INetwork::PacketSizeType)data.length();
			packet.header.length = length;
			network.WriteConnectionPacket(connection, data.data(), length, packet);
		} else {
			size_t length = data.length();
			network.WriteConnection(connection, data.data(), length);
		}

		network.Flush(connection);
	}
}

void Connection::GetAddress(IScript::Request& request) {
	String src, dst;
	network.GetConnectionInfo(connection, src, dst);

	request << begintable
		<< key("Source") << src
		<< key("Destination") << dst
		<< endtable;
}

void Connection::ReadHttpRequest(IScript::Request& request) {
	if (httpRequest != nullptr) {
		std::list<std::pair<String, String> > header;
		network.GetHttpRequestHeader(httpRequest, header);
		String uri = network.GetHttpRequestUri(httpRequest);
		String method = network.GetHttpRequestMethod(httpRequest);
		String data = network.GetHttpRequestData(httpRequest);

		request.DoLock();
		request << begintable
			<< key("Uri") << uri
			<< key("Method") << method
			<< key("Header") << header
			<< key("Data") << data
			<< endtable;
		request.UnLock();
	}
}

void Connection::WriteHttpRequest(const String& uri, const String& method, const std::list<std::pair<String, String> >& header, const String& data) {
	if (httpRequest != nullptr) {
		network.PrepareHttpRequest(httpRequest);
		network.SetHttpRequestMethod(httpRequest, method);
		network.SetHttpRequestUri(httpRequest, uri);
		network.SetHttpRequestHeader(httpRequest, header);
		network.SetHttpRequestData(httpRequest, data);
		network.MakeHttpRequest(httpRequest);
	}
}

void Connection::WriteHttpResponse(const String& data, int code, const String& reason, std::list<std::pair<String, String> >& header) {
	if (httpRequest != nullptr) {
		network.SetHttpRequestHeader(httpRequest, header);
		network.MakeHttpResponse(httpRequest, code, reason, data);
	}
}

