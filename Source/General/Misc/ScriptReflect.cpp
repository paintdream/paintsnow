#include "ScriptReflect.h"
#include <cassert>

using namespace PaintsNow;

std::unordered_map<Unique, ScriptReflect::Type> ScriptReflect::globalReflectParserMap;

namespace PaintsNow {
	static const ScriptReflect::Type& GetTypeInternal(const std::unordered_map<Unique, ScriptReflect::Type>& m, Unique typeID) {
		static ScriptReflect::Type dummy(String("<Unknown>"), nullptr);
		std::unordered_map<Unique, ScriptReflect::Type>::const_iterator p = m.find(typeID);

		p = m.find(typeID);
		if (p != m.end()) {
			assert(!(*p).second.name.empty());
			return (*p).second;
		} else {
			// Not registerd!
			assert(false);
			return dummy;
		}
	}
}

template <class T>
class Declare {
public:
	static void Register(const String& name, std::unordered_map<Unique, ScriptReflect::Type>& reflectParserMap) {
		static ScriptReflect::ValueParser<T> instance;
		singleton Unique u = UniqueType<T>::Get();
		reflectParserMap[u] = ScriptReflect::Type(name, &instance);
	}
};

ScriptReflect::ScriptReflect(IScript::Request& r, bool re, const std::unordered_map<Unique, Type>& m) : IReflect(true, true),  reflectParserMap(m), request(r), read(re) {
}

const ScriptReflect::Type& ScriptReflect::GetType(Unique typeID) const {
	return GetTypeInternal(reflectParserMap, typeID);
}

const std::unordered_map<Unique, ScriptReflect::Type>& ScriptReflect::GetGlobalMap() {
	// Register basic value parser
	static bool inited = false;
	if (!inited) {
		Declare<String>::Register("string", globalReflectParserMap);
		Declare<float>::Register("float", globalReflectParserMap);
		Declare<double>::Register("double", globalReflectParserMap);
		Declare<bool>::Register("bool", globalReflectParserMap);
		Declare<int8_t>::Register("int8_t", globalReflectParserMap);
		Declare<int16_t>::Register("int16_t", globalReflectParserMap);
		Declare<int32_t>::Register("int32_t", globalReflectParserMap);
		Declare<int64_t>::Register("int64_t", globalReflectParserMap);
		Declare<uint8_t>::Register("uint8_t", globalReflectParserMap);
		Declare<uint16_t>::Register("uint16_t", globalReflectParserMap);
		Declare<uint32_t>::Register("uint32_t", globalReflectParserMap);
		Declare<uint64_t>::Register("uint64_t", globalReflectParserMap);
		inited = true;
	}

	return globalReflectParserMap;
}

void ScriptReflect::Atom(Unique typeID, void* ptr) {
	const ValueParserBase* parser = GetType(typeID).parser;
	if (read) {
		parser->ReadValue(request, ptr);
	} else {
		parser->WriteValue(request, ptr);
	}
}

void ScriptReflect::Perform(const IReflectObject& s, void* base, Unique id) {
	if (s.IsBasicObject()) {
		Atom(id, base);
	} else {
		s(*this);
	}
}

void ScriptReflect::Property(IReflectObject& s, Unique typeID, Unique refTypeID, const char* name, void* base, void* ptr, const MetaChainBase* meta) {
	if (read) {
		request >> key(name);
	} else {
		request << key(name);
	}

	if (s.IsBasicObject()) {
		Atom(typeID, ptr);
	} else {
		if (s.IsIterator()) {
			IIterator& it = static_cast<IIterator&>(s);
			// uint64_t count;
			IScript::Request::ArrayStart ts;
			if (read) {
				request >> ts;
				it.Initialize((size_t)ts.count);
				// count = ts.count;
			} else {
				request << begintable;
				// count = (uint64_t)it.GetTotalCount();
			}

			if (it.IsElementBasicObject()) {
				while (it.Next()) {
					(*reinterpret_cast<IReflectObject*>(it.Get()))(*this);
				}
			} else {
				while (it.Next()) {
					Atom(typeID, it.Get());
				}
			}

			if (read) {
				request >> endarray;
			} else {
				request << endarray;
			}
		} else {
			if (read) {
				request >> begintable;
				s(*this);
				request >> endtable;
			} else {
				request << begintable;
				s(*this);
				request << endtable;
			}
		}
	}
}

void ScriptReflect::Method(const char* name, const TProxy<>* p, const IReflect::Param& retValue, const std::vector<IReflect::Param>& params, const MetaChainBase* meta) {}

Bridge::Bridge(IThread& thread) : ISyncObject(thread) {}
Bridge::~Bridge() {}

Proxy::Proxy(Tunnel* host, const TProxy<>* p) : hostTunnel(host), routine(p) {}

void Proxy::OnCall(IScript::Request& request) {
	hostTunnel->ForwardCall(routine, request);
}

Tunnel::Tunnel(Bridge* b, IReflectObject* h) : bridge(b), host(h) {}

Tunnel::~Tunnel() {
	delete host;
}

TObject<IReflect>& Tunnel::operator () (IReflect& reflect) {
	BaseClass::operator () (reflect);
	return *this;
}

void Tunnel::Dump(IScript::Request& request) {
	ObjectDumper dumper(request, *this, bridge->GetReflectMap());
	request.DoLock();
	request << begintable;
	// request << key("__tunnel") << this; // keep reference
	// bridge->Dump(request, *this, host);
	(*host)(dumper);
	request << endtable;
	request.UnLock();
}

void Tunnel::ForwardCall(const TProxy<>* p, IScript::Request& request) {
	bridge->Call(host, p, request);
}

Proxy& Tunnel::NewProxy(const TProxy<>* p) {
	proxy.emplace_back(Proxy(this, p));
	return proxy.back();
}

IReflectObject* Tunnel::GetHost() const {
	return host;
}

ObjectDumper::ObjectDumper(IScript::Request& r, Tunnel& t, const std::unordered_map<Unique, Type>& m) : ScriptReflect(r, false, m), request(r), tunnel(t) {}

void ObjectDumper::Property(IReflectObject& s, Unique typeID, Unique refTypeID, const char* name, void* base, void* ptr, const MetaChainBase* meta) {
	// TODO: place code here
	ScriptReflect::Property(s, typeID, refTypeID, name, base, ptr, meta);
}

void ObjectDumper::Method(const char* name, const TProxy<>* p, const IReflect::Param& retValue, const std::vector<IReflect::Param>& params, const MetaChainBase* meta) {
	if (!read) {
		Proxy& proxy = tunnel.NewProxy(p);
		request << key(name) << request.Adapt(Wrap(&proxy, &Proxy::OnCall));

		String extraKey = String("meta$") + name;
		// generate parameter info
		request << key(extraKey.c_str()) << begintable
			<< key("retval") << begintable
			<< key("type") << GetType(retValue).name
			<< endtable
			<< key("params") << beginarray;

		for (size_t i = 0; i < params.size(); i++) {
			request << begintable
				<< key("type") << GetType(params[i]).name
				<< key("name") << params[i].name
				<< endtable;

			/*
			if (!params[i].parameters.empty()) {
				const String& object = "<Object>";
				size_t pos = type.find(object);
				assert(pos != String::npos);
				type.replace(pos, object.size(), params[i].parameters);
			}*/
		}

		request << endarray << endtable;
	}
}

