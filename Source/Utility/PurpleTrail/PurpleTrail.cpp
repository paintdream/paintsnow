#include "PurpleTrail.h"
#include "../../Core/Template/TBuffer.h"
#include "../../Core/Driver/Profiler/Optick/optick.h"

using namespace PaintsNow;

struct InspectCustomStructure : public IReflect {
	InspectCustomStructure(IScript::Request& r, IReflectObject& obj) : request(r), IReflect(true, false) {
		request << begintable;
		String name, count;
		InspectCustomStructure::FilterType(obj.GetUnique()->GetName(), name, count);

		request << key("Type") << name;
		request << key("Optional") << false;
		request << key("Pointer") << false;
		request << key("List") << false;
		request << key("Fields") << begintable;
		obj(*this);
		request << endtable;
		request << endtable;
	}

	void Property(IReflectObject& s, Unique typeID, Unique refTypeID, const char* name, void* base, void* ptr, const MetaChainBase* meta) override {
		singleton Unique typedBaseType = UniqueType<IScript::MetaVariable::TypedBase>::Get();
		for (const MetaChainBase* t = meta; t != nullptr; t = t->GetNext()) {
			const MetaNodeBase* node = t->GetNode();
			if (!node->IsBasicObject() && node->GetUnique() == typedBaseType) {
				IScript::MetaVariable::TypedBase* entry = const_cast<IScript::MetaVariable::TypedBase*>(static_cast<const IScript::MetaVariable::TypedBase*>(node));
				request << key(entry->name.empty() ? name : entry->name);
				InspectCustomStructure::ProcessMember(request, typeID, s.IsIterator(), false);
			}
		}
	}

	void Method(const char* name, const TProxy<>* p, const Param& retValue, const std::vector<Param>& params, const MetaChainBase* meta) override {}

	static bool FilterType(const String& name, String& ret, String& count) {
		// parse name
		if (name == UniqueType<String>::Get()->GetName() || name == UniqueType<Bytes>::Get()->GetName()) {
			ret = "string";
			return true;
		} else if (name.find("std::vector") == 0 || name.find("std::pair") == 0 || name.find("std::list") == 0) {
			ret = "vector";
			return false;
		} else {
			ret = "";
			bool inword = false;
			for (size_t i = 0; i < name.length(); i++) {
				char ch = name[i];
				if (isalnum(ch) || ch == '_' || ch == ':') {
					if (!inword) ret = "";
					ret += ch;
					inword = true;
				} else {
					inword = false;
				}
			}
		}

		static const String typePrefix = "TType";
		String::size_type pos = name.find(typePrefix);
		String::size_type end = pos;
		if (pos != String::npos) {
			pos += typePrefix.length();
			end += typePrefix.length();
			while (end < name.length() && isdigit(name[end])) end++;
		}

		count = "";
		if (end != String::npos) {
			count = name.substr(pos, end - pos);
		}

		return true;
	}

	static void ProcessMember(IScript::Request& request, Unique type, bool isIterator, bool optional) {
		// filter TType
		String typeName = type->GetName();

		bool isDelegate = typeName.find("PaintsNow::BaseDelegate") != 0 || typeName.find("PaintsNow::Delegate<") != 0;
		String name, count;
		bool subfield = InspectCustomStructure::FilterType(typeName, name, count);

		if (type->IsCreatable()) {
			IReflectObject* obj = type->Create();
			if (obj != nullptr) {
				InspectCustomStructure(request, *obj);
				obj->Destroy();
			} else {
				request << begintable;
				request << key("Pointer") << false;
				request << key("Optional") << false;
				request << key("List") << false;
				request << key("Fields") << beginarray << endarray;
				request << key("Type") << String("ErrorType");
				request << endtable;
				assert(false);
			}
		} else {
			request << begintable;
			request << key("Optional") << (isDelegate && optional);
			request << key("Pointer") << count.empty();
			request << key("List") << isIterator;
			request << key("Fields") << beginarray;

			if (subfield) {
				int c = atoi(count.c_str());
				for (int i = 0; i < c; i++) {
					request << name;
				}
			}

			request << endarray;
			request << key("Type") << String(name + count);
			request << endtable;
		}
	}

	IScript::Request& request;
};

class PurpleTrail::IInspectPrimitive {
public:
	virtual void Write(IScript::Request& request, const void* p) = 0;
	virtual void Read(IScript::Request& request, void* p) = 0;
};

template <class T>
class InspectPrimitiveImpl : public PurpleTrail::IInspectPrimitive {
public:
	void Write(IScript::Request& request, const void* p) override {
		const T* t = reinterpret_cast<const T*>(p);
		request << *t;
	}

	void Read(IScript::Request& request, void* p) override {
		T& t = *reinterpret_cast<T*>(p);
		request >> t;
	}
};

#define REGISTER_PRIMITIVE(f) \
	{ static InspectPrimitiveImpl<f> prim_##f; \
	mapInspectors[UniqueType<f>::Get()] = &prim_##f; }

struct InspectProcs : public IReflect {
	InspectProcs(std::unordered_map<Unique, PurpleTrail::IInspectPrimitive*>& mapIns, IScript::Request& r, IReflectObject& obj) : mapInspectors(mapIns), request(r), IReflect(false, true, true, true) {
		request << begintable;
		obj(*this);
		request << endtable;
	}

	void Enum(size_t value, Unique id, const char* name, const MetaChainBase* meta) override {
		request << key(name) << safe_cast<int32_t>(value);
	}

	void WriteReflectObject(IReflectObjectComplex* p) {
		if (p != nullptr) {
			if (p->QueryInterface(UniqueType<IScript::Object>()) != nullptr) {
				request << static_cast<IScript::Object*>(p);
			} else {
				(*p)(*this);
			}
		} else {
			request << nil;
		}
	}

	void Property(IReflectObject& s, Unique typeID, Unique refTypeID, const char* name, void* base, void* ptr, const MetaChainBase* meta) override {
		if (s.IsBasicObject()) {
			if (typeID != refTypeID) {
				request << key(name);
				WriteReflectObject(*reinterpret_cast<IReflectObjectComplex**>(ptr));
			} else {
				std::unordered_map<Unique, PurpleTrail::IInspectPrimitive*>::const_iterator p = mapInspectors.find(typeID);
				if (p != mapInspectors.end()) {
					request << key(name);
					(*p).second->Write(request, ptr);
				}
			}
		} else if (s.IsIterator()) {
			IIterator& it = static_cast<IIterator&>(s);
			if (it.IsElementBasicObject()) {
				Unique unique = it.GetElementUnique();
				if (unique != refTypeID) {
					request << key(name) << beginarray;
					while (it.Next()) {
						WriteReflectObject(*reinterpret_cast<IReflectObjectComplex**>(it.Get()));
					}
					request << endarray;
				} else {
					std::unordered_map<Unique, PurpleTrail::IInspectPrimitive*>::const_iterator p = mapInspectors.find(it.GetElementUnique());
					if (p != mapInspectors.end()) {
						request << key(name) << beginarray;

						while (it.Next()) {
							((*p).second)->Write(request, it.Get());
						}

						request << endarray;
					}
				}
			} else {
				request << key(name) << beginarray;
				while (it.Next()) {
					IReflectObjectComplex& sub = *reinterpret_cast<IReflectObjectComplex*>(it.Get());
					WriteReflectObject(&sub);
				}
				request << endarray;
			}
		} else {
			request << key(name);
			WriteReflectObject(static_cast<IReflectObjectComplex*>(&s));
		}
	}

	void Method(const char* name, const TProxy<>* p, const Param& retValue, const std::vector<Param>& params, const MetaChainBase* meta) override {
		// convert params ...
		while (meta != nullptr) {
			const MetaNodeBase* node = meta->GetNode();
			singleton Unique ScriptMethodUnique = UniqueType<IScript::MetaMethod::TypedBase>::Get();
			if (node->GetUnique() == ScriptMethodUnique) {
				const IScript::MetaMethod::TypedBase* entry = static_cast<const IScript::MetaMethod::TypedBase*>(node);
				request << key(entry->name.empty() ? name : entry->name) << begintable << key("Params") << beginarray;
				for (size_t i = 1; i < params.size(); i++) {
					InspectCustomStructure::ProcessMember(request, params[i].decayType, false, true);
				}

				request << endarray;
				request << key("Returns");
				request << begintable;
				InspectCustomStructure::ProcessMember(request, retValue.decayType, false, true);
				request << endtable;
				request << endtable;
				break;
			}

			meta = meta->GetNext();
		}
	}

	std::unordered_map<Unique, PurpleTrail::IInspectPrimitive*>& mapInspectors;
	IScript::Request& request;
};

void PurpleTrail::RequestInspect(IScript::Request& request, IScript::BaseDelegate d) {
	CHECK_REFERENCES_NONE();
	OPTICK_EVENT();

	if (d.GetRaw() != nullptr) {
		InspectProcs procs(mapInspectors, request, *d.GetRaw());
	}
}

template <bool set>
class PropertyGetSet : public IReflect {
public:
	PropertyGetSet(std::unordered_map<Unique, PurpleTrail::IInspectPrimitive*>& inspectors, IScript::Request& r, const std::vector<String>& p) : IReflect(true, false, true, false), mapInspectors(inspectors), request(r), path(p), level(0), finished(false) {}

	void SyncProperty(PurpleTrail::IInspectPrimitive* p, void* ptr) {
		if (set) {
			p->Write(request, ptr);
		} else {
			p->Read(request, ptr);
		}
	}

	void Property(IReflectObject& s, Unique typeID, Unique refTypeID, const char* name, void* base, void* ptr, const MetaChainBase* meta) override {
		if (finished || strcmp(name, path[level].c_str()) != 0) {
			return;
		}

		if (!s.IsBasicObject()) {
			if (s.IsIterator()) {
				IIterator& it = static_cast<IIterator&>(s);
				if (it.IsElementBasicObject()) {
					Unique unique = it.GetElementUnique();
					if (level + 1 == path.size()) { // last key, must be array assignment
						if (unique == refTypeID) {
							std::unordered_map<Unique, PurpleTrail::IInspectPrimitive*>::const_iterator p = mapInspectors.find(unique);

							if (p != mapInspectors.end()) {
								if (set) {
									IScript::Request::ArrayStart as;
									request >> as;
									it.Initialize(as.count);

									while (it.Next()) {
										SyncProperty((*p).second, it.Get());
									}

									request >> endarray;
								} else {
									request << beginarray;

									while (it.Next()) {
										SyncProperty((*p).second, it.Get());
									}

									request << endarray;
								}

								finished = true;
							}
						}
					} else { // element assignment
						level++;
						size_t index = (size_t)atoi(path[level].c_str());
						size_t i = 0;
						while (it.Next() && i++ < index) {}

						if (i == index) {
							if (unique != refTypeID) {
								level++;
								(**reinterpret_cast<IReflectObjectComplex**>(it.Get()))(*this);
								level--;
							} else {
								std::unordered_map<Unique, PurpleTrail::IInspectPrimitive*>::const_iterator p = mapInspectors.find(it.GetElementUnique());
								if (p != mapInspectors.end()) {
									SyncProperty((*p).second, ptr);
									finished = true;
								}
							}
						}

						level--;
					}
				} else {
					level++;
					size_t index = (size_t)atoi(path[level].c_str());
					size_t i = 0;
					while (it.Next() && i++ < index) {}

					if (i == index) {
						level++;
						(**reinterpret_cast<IReflectObjectComplex**>(it.Get()))(*this);
						level--;
					}

					level--;
				}
			} else {
				level++;
				s(*this);
				level--;
			}
		} else {
			std::unordered_map<Unique, PurpleTrail::IInspectPrimitive*>::const_iterator p = mapInspectors.find(typeID);
			if (p != mapInspectors.end()) {
				SyncProperty((*p).second, ptr);
				finished = true;
			}
		}
	}

	std::unordered_map<Unique, PurpleTrail::IInspectPrimitive*>& mapInspectors;
	IScript::Request& request;
	const std::vector<String>& path;
	uint32_t level;
	bool finished;
};

void PurpleTrail::RequestGetValue(IScript::Request& request, IScript::BaseDelegate d, const std::vector<String>& path) {
	CHECK_REFERENCES_NONE();
	OPTICK_EVENT();

	if (path.empty()) return;

	if (d.GetRaw() != nullptr) {
		PropertyGetSet<false> getset(mapInspectors, request, path);
		(*d.GetRaw())(getset);
	}
}

void PurpleTrail::RequestSetValue(IScript::Request& request, IScript::BaseDelegate d, const std::vector<String>& path, IScript::Request::Arguments args) {
	CHECK_REFERENCES_NONE();
	OPTICK_EVENT();

	if (path.empty()) return;

	if (d.GetRaw() != nullptr) {
		PropertyGetSet<true> getset(mapInspectors, request, path);
		(*d.GetRaw())(getset);
	}
}

PurpleTrail::PurpleTrail(IThread& t) : ISyncObject(t) {
	REGISTER_PRIMITIVE(String);
	REGISTER_PRIMITIVE(Bytes);
	REGISTER_PRIMITIVE(bool);
	REGISTER_PRIMITIVE(size_t);
	REGISTER_PRIMITIVE(int8_t);
	REGISTER_PRIMITIVE(uint8_t);
	REGISTER_PRIMITIVE(int16_t);
	REGISTER_PRIMITIVE(uint16_t);
	REGISTER_PRIMITIVE(int32_t);
	REGISTER_PRIMITIVE(uint32_t);

	REGISTER_PRIMITIVE(int64_t);
	REGISTER_PRIMITIVE(uint64_t);
	REGISTER_PRIMITIVE(float);
	REGISTER_PRIMITIVE(double);

	REGISTER_PRIMITIVE(Int2);
	REGISTER_PRIMITIVE(Int3);
	REGISTER_PRIMITIVE(Int4);
	REGISTER_PRIMITIVE(Float2);
	REGISTER_PRIMITIVE(Float2Pair);
	REGISTER_PRIMITIVE(Float3);
	REGISTER_PRIMITIVE(Float3Pair);
	REGISTER_PRIMITIVE(Float4);
	REGISTER_PRIMITIVE(Float4Pair);
	REGISTER_PRIMITIVE(Double2);
	REGISTER_PRIMITIVE(Double2Pair);
	REGISTER_PRIMITIVE(Double3);
	REGISTER_PRIMITIVE(Double3Pair);
	REGISTER_PRIMITIVE(Double4);
	REGISTER_PRIMITIVE(Double4Pair);
	REGISTER_PRIMITIVE(MatrixFloat3x3);
	REGISTER_PRIMITIVE(MatrixFloat4x4);
}

PurpleTrail::~PurpleTrail() {}

TObject<IReflect>& PurpleTrail::operator () (IReflect& reflect) {
	BaseClass::operator () (reflect);
	if (reflect.IsReflectMethod()) {
		ReflectMethod(RequestInspect)[ScriptMethodLocked = "Inspect"];
		ReflectMethod(RequestGetValue)[ScriptMethodLocked = "GetValue"];
		ReflectMethod(RequestSetValue)[ScriptMethodLocked = "SetValue"];
	}

	return *this;
}

