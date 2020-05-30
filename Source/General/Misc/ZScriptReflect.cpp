#include "ZScriptReflect.h"
#include <cassert>

using namespace PaintsNow;

std::map<Unique, ZScriptReflect::Type> ZScriptReflect::globalReflectParserMap;

namespace PaintsNow {
	static const ZScriptReflect::Type& GetTypeInternal(const std::map<Unique, ZScriptReflect::Type>& m, Unique typeID) {
		static ZScriptReflect::Type dummy(String("<Unknown>"), nullptr);
		std::map<Unique, ZScriptReflect::Type>::const_iterator p = m.find(typeID);

		p = m.find(typeID);
		if (p != m.end()) {
			assert(!p->second.name.empty());
			return p->second;
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
	static void Register(const String& name, std::map<Unique, ZScriptReflect::Type>& reflectParserMap) {
		static ZScriptReflect::ValueParser<T> instance;
		static Unique u = UniqueType<T>::Get();
		reflectParserMap[u] = ZScriptReflect::Type(name, &instance);
	}
};

ZScriptReflect::ZScriptReflect(IScript::Request& r, bool re, const std::map<Unique, Type>& m) : IReflect(true, true),  reflectParserMap(m), request(r), read(re) {
}

const ZScriptReflect::Type& ZScriptReflect::GetType(Unique typeID) const {
	return GetTypeInternal(reflectParserMap, typeID);
}

const std::map<Unique, ZScriptReflect::Type>& ZScriptReflect::GetGlobalMap() {
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

void ZScriptReflect::Atom(Unique typeID, void* ptr) {
	const ValueParserBase* parser = GetType(typeID).parser;
	if (read) {
		parser->ReadValue(request, ptr);
	} else {
		parser->WriteValue(request, ptr);
	}
}

void ZScriptReflect::Perform(const IReflectObject& s, void* base, Unique id) {
	if (s.IsBasicObject()) {
		Atom(id, base);
	} else {
		s(*this);
	}
}

void ZScriptReflect::Property(IReflectObject& s, Unique typeID, Unique refTypeID, const char* name, void* base, void* ptr, const MetaChainBase* meta) {
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

			if (it.GetPrototype().IsBasicObject()) {
				while (it.Next()) {
					(*reinterpret_cast<IReflectObject*>(it.Get()))(*this);
				}
			} else {
				while (it.Next()) {
					Atom(typeID, it.Get());
				}
			}

			if (read) {
				request >> endtable;
			} else {
				request << endtable;
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

void ZScriptReflect::Method(Unique typeID, const char* name, const TProxy<>* p, const IReflect::Param& retValue, const std::vector<IReflect::Param>& params, const MetaChainBase* meta) {}


