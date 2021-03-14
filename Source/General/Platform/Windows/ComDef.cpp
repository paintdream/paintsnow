#include "ComDef.h"
#include "../../../General/Template/TMaskType.h"
#include "../../../Core/Interface/IScript.h"
#include "ComDispatch.h"
#include "ComBridge.h"
using namespace PaintsNow;

#pragma warning(disable:4996)

namespace PaintsNow {
	String ToCoreString(const BSTR& str) {
		if (str != nullptr) {
			const char* p = (const char*)(str);
			String w(p, ::SysStringByteLen(str));
			return SystemToUtf8(w);
		} else {
			return "";
		}
	}

	ComVariant FromCoreString(const String& str) {
		String w = Utf8ToSystem(str);
		return (BSTR)w.data();
	}
}

template <class T>
struct MapType {
	enum { VtType = VT_PTR, VtTypeTarget = VtType };
};

template <>
struct MapType<String> {
	typedef BSTR target;
	enum { VtType = VT_BSTR, VtTypeTarget = VtType };
	static ComVariant From(const String& str) {
		return FromCoreString(str);
	}

	static String To(ComBridge* br, const ComVariant& str) {
		if (str->vt == VT_EMPTY)
			return "";
		else
			return ToCoreString(str->bstrVal);
	}
};

template <>
struct MapType<int8_t> {
	typedef CHAR target;
	enum { VtType = VT_I1, VtTypeTarget = VtType };
	static ComVariant From(const int8_t& s) {
		return s;
	}

	static int8_t To(ComBridge* br, const ComVariant& str) {
		return str->cVal;
	}
};

template <>
struct MapType<int16_t> {
	typedef SHORT target;
	enum { VtType = VT_I2, VtTypeTarget = VtType };
	static ComVariant From(const int16_t& s) {
		return s;
	}

	static int16_t To(ComBridge* br, const ComVariant& str) {
		return str->iVal;
	}
};

template <>
struct MapType<int32_t> {
	typedef INT target;
	enum { VtType = VT_I4, VtTypeTarget = VtType };
	static ComVariant From(const int32_t& s) {
		return s;
	}

	static int16_t To(ComBridge* br, const ComVariant& str) {
		return str->intVal;
	}
};

template <>
struct MapType<int64_t> {
	typedef LONGLONG target;
	enum { VtType = VT_I8, VtTypeTarget = VtType };
	static ComVariant From(const int64_t& s) {
		return s;
	}

	static int64_t To(ComBridge* br, const ComVariant& str) {
		return str->llVal;
	}
};

template <>
struct MapType<long> {
	typedef LONG target;
	enum { VtType = VT_I4, VtTypeTarget = VtType }; // x64 ?!
	static ComVariant From(const long& s) {
		return s;
	}

	static int16_t To(ComBridge* br, const ComVariant& str) {
		return str->intVal;
	}
};

template <>
struct MapType<uint8_t> {
	typedef UCHAR target;
	enum { VtType = VT_UI1, VtTypeTarget = VtType };
	static ComVariant From(const uint8_t& s) {
		return s;
	}

	static uint8_t To(ComBridge* br, const ComVariant& str) {
		return str->bVal;
	}
};

template <>
struct MapType<uint16_t> {
	typedef USHORT target;
	enum { VtType = VT_UI2, VtTypeTarget = VtType };
	static ComVariant From(const uint16_t& s) {
		return s;
	}

	static int16_t To(ComBridge* br, const ComVariant& str) {
		return str->uiVal;
	}
};

template <>
struct MapType<uint32_t> {
	typedef UINT target;
	enum { VtType = VT_UI4, VtTypeTarget = VtType };
	static ComVariant From(const uint32_t& s) {
		return s;
	}

	static int32_t To(ComBridge* br, const ComVariant& str) {
		return str->uintVal;
	}
};

template <>
struct MapType<uint64_t> {
	typedef LONGLONG target;
	enum { VtType = VT_UI8, VtTypeTarget = VtType };
	static ComVariant From(const uint64_t& s) {
		return s;
	}

	static int64_t To(ComBridge* br, const ComVariant& str) {
		return str->ullVal;
	}
};

template <>
struct MapType<bool> {
	typedef BOOL target;
	enum { VtType = VT_BOOL, VtTypeTarget = VtType };
	static ComVariant From(const bool& s) {
		return s;
	}

	static bool To(ComBridge* br, const ComVariant& str) {
		return str->boolVal != 0;
	}
};

template <>
struct MapType<float> {
	typedef FLOAT target;
	enum { VtType = VT_R4, VtTypeTarget = VtType };
	static ComVariant From(const float& s) {
		return s;
	}

	static float To(ComBridge* br, const ComVariant& str) {
		return str->fltVal;
	}
};

template <>
struct MapType<double> {
	typedef DOUBLE target;
	enum { VtType = VT_R8, VtTypeTarget = VtType };
	static ComVariant From(const double& s) {
		return s;
	}

	static DOUBLE To(ComBridge* br, const ComVariant& str) {
		return str->dblVal;
	}
};

template <>
struct MapType<Void> {
	typedef Void target;
	enum { VtType = VT_VOID, VtTypeTarget = VtType };
	static ComVariant From(const Void& s) {
		return ComVariant();
	}

	static Void To(ComBridge* br, const ComVariant& str) {
		return Void();
	}
};

enum { CAST_ARRAY = 1, CAST_REF = 2, CAST_SELF = 3, CAST_SIZE = 4 };

BEGIN_TYPEMASK(TMaskCast, CAST_SIZE)

static ComVariant dummy;

template <class T>
static String AntiBugGetName(const String& alias, T) {
	return T::GetName(alias);
}

template <>
struct TMaskCast<CAST_SELF> {
	template <class T>
	struct Impl : public AutoVariant {
		typedef T type;
		enum { CastType = CAST_SELF };
		T self;
		ComVariant& var;
		Impl(ComVariant& v = dummy) : var(v) {}

		virtual bool IsOutput() const {
			return false;
		}

		virtual void Import(ComBridge* br, IScript::Request& request) {
			self = MapType<T>::To(br, var);
			request << self;
		}

		virtual void Export(ComBridge* br, IScript::Request& request) {
			self = MapType<T>::To(br, var);
			request >> self;
			var = ComVariant(MapType<T>::From(self));
		}

		virtual void Detach(IScript::Request& request) {}

		static String GetName(const String& alias) {
			return alias;
		}
	};
};

template <>
struct TMaskCast<CAST_REF> {
	template <class T>
	struct Impl : public AutoVariant {
		enum { CastType = CAST_REF };
		typedef T type;
		T self;
		ComVariant& var;
		IScript::Request::Ref handle;
		Impl(ComVariant& v = dummy) : var(v) {
			// TODO: var?
		}

		virtual bool IsOutput() const {
			return MapType<T::type>::VtType != VT_USERDEFINED;
		}

		virtual ~Impl() {
			assert(!handle);
		}

		virtual void Detach(IScript::Request& request) {
			if (handle) {
				request.Dereference(handle);
			}
		}
		virtual void Export(ComBridge* br, IScript::Request& request) {
			assert(!handle);
			if (MapType<T::type>::VtType == VT_USERDEFINED) {
				self.Export(br, request);
			} else {
				request >> handle;
				request.Push();
				request << handle;
				request >> begintable;
				self.Export(br, request);
				/*
				if (T::CastType == CAST_ARRAY) {
					var.vt = VT_ARRAY | VT_BYREF;
				} else if (T::CastType == CAST_REF) {
					// COM bug, no way to write back
					//	var.vt = MapType<T::type>::VtType | VT_ARRAY; // SAFEARRAY** , cannot help but make function call success.
					var.vt = self.var.vt | VT_BYREF;
				}
				*/

				((VARIANT&)var).vt = ((VARIANT&)self.var).vt | VT_BYREF;
				((VARIANT&)var).pparray = &((VARIANT&)self.var).parray;
				request << endtable;
				request.Pop();
			}
		}

		virtual void Import(ComBridge* br, IScript::Request& request) {
			size_t t = MapType<T::type>::VtType;
			// flush self value
			if (MapType<T::type>::VtType == VT_USERDEFINED) {
				self.Import(br, request);
			} else {
				assert(handle);
				request.Push();
				request << handle;
				request >> begintable;
				self.Import(br, request);
				request >> endtable;
				request.Pop();
			}
		}

		static String GetName(const String& alias) {
			return AntiBugGetName(alias, T()) + "*";
		}
	};
};

template <>
struct TMaskCast<CAST_ARRAY> {
	template <class T>
	struct Impl : AutoVariant {
		typedef T type;
		enum { CastType = CAST_REF };
		std::list<T> container;
		ComVariant& var;
		Impl(ComVariant& v = dummy) : var(v) {}

		virtual bool IsOutput() const {
			return false;
		}

		virtual void Detach(IScript::Request& request) {}
		virtual void Export(ComBridge* bridge, IScript::Request& request) {
			IScript::Request::TableStart ts;
			request >> ts;
			SAFEARRAYBOUND bound = { (ULONG)ts.count, 0 };

			SAFEARRAY* safeArray = ::SafeArrayCreate(T::CastType == CAST_ARRAY ? VT_ARRAY : T::CastType == CAST_REF ? VT_PTR : MapType<T::type>::VtTypeTarget, 1, &bound);
			container.clear();
			for (size_t i = 0; i < ts.count; i++) {
				long index = (long)i;
				container.emplace_back(T());
				container.back().Export(bridge, request);
				::SafeArrayPutElement(safeArray, &index, &((VARIANT&)(container.back().var)).intVal);
			}

			request >> endtable;

			VARIANT& v = (VARIANT&)var;
			v.vt = VT_ARRAY | MapType<T::type>::VtTypeTarget;
			v.parray = safeArray;
		}

		virtual void Import(ComBridge* br, IScript::Request& request) {
			SAFEARRAY* safeArray = ((VARIANT&)var).parray;
			LONG lower;
			::SafeArrayGetLBound(safeArray, 1, &lower);
			LONG upper;
			::SafeArrayGetUBound(safeArray, 1, &upper);
			request << begintable;
			std::list<T>::iterator it = container.begin();
			for (LONG index = lower; index <= upper && it != container.end(); index++) {
				::SafeArrayGetElement(safeArray, &index, &((VARIANT&)((*it).var)).intVal);
				it->Import(br, request);
				++it;
			}
			request << endtable;
		}

		static String GetName(const String& alias) {
			return String("Array<") + AntiBugGetName(alias, T()) + ">";
		}
	};
};

END_TYPEMASK(TMaskCast, TMaskType)

template <class T, size_t mask>
class ActionBase : public AutoVariantHandler {
public:
	typedef typename TMaskType<T, mask>::type type;
	enum { WrapType = mask % CAST_SIZE };
	ActionBase() {}

	virtual Unique GetValueType() const {
		return UniqueType<type>::Get();
	}

	virtual AutoVariant* Create(ComVariant& variant) const {
		return new type(variant);
	}

	void WriteValue(IScript::Request& request, const void* base) const {
		// Not implemented
		assert(false);
		// assume base is an object of type ComVariant
		// type::Write(request, var, *reinterpret_cast<ComVariant*>(const_cast<void*>(base)));
	}

	void ReadValue(IScript::Request& request, void* base) const {
		// Not implemented
		assert(false);
		// type::Read(request, var, *reinterpret_cast<ComVariant*>(base));
	}
};

/*
enum VARENUM
{
VT_EMPTY	= 0,
VT_nullptr	= 1,
VT_I2	= 2,
VT_I4	= 3,
VT_R4	= 4,
VT_R8	= 5,
VT_CY	= 6,
VT_DATE	= 7,
VT_BSTR	= 8,
VT_DISPATCH	= 9,
VT_ERROR	= 10,
VT_BOOL	= 11,
VT_VARIANT	= 12,
VT_UNKNOWN	= 13,
VT_DECIMAL	= 14,
VT_I1	= 16,
VT_UI1	= 17,
VT_UI2	= 18,
VT_UI4	= 19,
VT_I8	= 20,
VT_UI8	= 21,
VT_INT	= 22,
VT_UINT	= 23,
VT_VOID	= 24,
VT_HRESULT	= 25,
VT_PTR	= 26,
VT_SAFEARRAY	= 27,
VT_CARRAY	= 28,
VT_USERDEFINED	= 29,
VT_LPSTR	= 30,
VT_LPWSTR	= 31,
VT_RECORD	= 36,
VT_INT_PTR	= 37,
VT_UINT_PTR	= 38,
VT_FILETIME	= 64,
VT_BLOB	= 65,
VT_STREAM	= 66,
VT_STORAGE	= 67,
VT_STREAMED_OBJECT	= 68,
VT_STORED_OBJECT	= 69,
VT_BLOB_OBJECT	= 70,
VT_CF	= 71,
VT_CLSID	= 72,
VT_VERSIONED_STREAM	= 73,
VT_BSTR_BLOB	= 0xfff,
VT_VECTOR	= 0x1000,
VT_ARRAY	= 0x2000,
VT_BYREF	= 0x4000,
VT_RESERVED	= 0x8000,
VT_ILLEGAL	= 0xffff,
VT_ILLEGALMASKED	= 0xfff,
VT_TYPEMASK	= 0xfff
} ;
*/

#define ADD_MASK(t, m) \
	(((((size_t)t & ~VT_TYPEMASK) * CAST_SIZE) + (size_t)m * (VT_TYPEMASK + 1)) | ((size_t)t & VT_TYPEMASK))

template <size_t level>
struct RegType {
	template <size_t base>
	struct Based {
		template <class T, size_t type>
		struct Reg {
			static void Register(std::unordered_map<size_t, Unique>& typeMap, std::unordered_map<Unique, ScriptReflect::Type>& reflectMap, const String& alias) {
				typedef typename ActionBase<T, base> Action;
				typedef typename Action::type Type;
				static Action action;
				Unique typeID = UniqueType<Action::type>::Get();
				reflectMap[typeID] = ScriptReflect::Type(Type::GetName(alias), &action);
				typeMap[type] = typeID;
				// printf("[%8X] To [%8X] = %s\n", type, typeID, Type::GetName(alias).c_str());

				// Next generation
				RegType<level - 1>::Based<(base* CAST_SIZE + CAST_ARRAY)>::Reg<T, ADD_MASK(type, CAST_ARRAY)>::Register(typeMap, reflectMap, alias);
				RegType<level - 1>::Based<(base* CAST_SIZE + CAST_REF)>::Reg<T, ADD_MASK(type, CAST_REF)>::Register(typeMap, reflectMap, alias);
			}
		};
	};
};

template <>
struct RegType<0> {
	template <size_t base>
	struct Based {
		template <class T, size_t type>
		struct Reg {
			static void Register(std::unordered_map<size_t, Unique>& typeMap, std::unordered_map<Unique, ScriptReflect::Type>& reflectMap, const String& alias) {}
		};
	};
};

template <class T>
struct Declare {
	static void Register(std::unordered_map<size_t, Unique>& typeMap, std::unordered_map<Unique, ScriptReflect::Type>& reflectMap) {
		RegType<3>::Based<CAST_SELF>::Reg<T, MapType<T>::VtType>::Register(typeMap, reflectMap, reflectMap[UniqueType<T>::Get()].name);
	}
};

namespace PaintsNow {
	size_t GenType(HREFTYPE& refType, String& userDefinedName, ITypeInfo* typeInfo, const TYPEDESC& desc) {
		VARTYPE vt = desc.vt;
		assert(vt != VT_CARRAY); // Cannot process it now
		if (vt == VT_PTR) { // BYREF
			size_t t = GenType(refType, userDefinedName, typeInfo, *desc.lptdesc);
			t = ADD_MASK(t, CAST_REF);
			return t;
		} else if (vt == VT_SAFEARRAY) {
			size_t t = GenType(refType, userDefinedName, typeInfo, desc.lpadesc->tdescElem);
			t = ADD_MASK(t, CAST_ARRAY);
			return t;
		} else if (vt == VT_USERDEFINED) {
			HREFTYPE h = desc.hreftype;
			ITypeInfo* w;
			if (SUCCEEDED(typeInfo->GetRefTypeInfo(h, &w))) {
				BSTR bstrName;
				w->GetDocumentation(MEMBERID_NIL, &bstrName, nullptr, nullptr, nullptr);
				userDefinedName = ToCoreString(bstrName);
				refType = h;
				w->Release();

				::SysFreeString(bstrName);
			}

			return vt;
		} else {
			return vt;
		}
	}

	void InitDPIAware() {
		typedef BOOL(WINAPI* SetProcessDPIAware)();
		SetProcessDPIAware proc = (SetProcessDPIAware)::GetProcAddress(::GetModuleHandleA("user32.dll"), "SetProcessDPIAware");
		if (proc != nullptr) {
			proc();
		}
	}

	void InitComReflectMap(std::unordered_map<size_t, Unique>& typeMap, std::unordered_map<Unique, ScriptReflect::Type>& reflectMap) {
		reflectMap = ScriptReflect::GetGlobalMap();
		Declare<String>::Register(typeMap, reflectMap);
		Declare<int8_t>::Register(typeMap, reflectMap);
		Declare<int16_t>::Register(typeMap, reflectMap);
		Declare<int32_t>::Register(typeMap, reflectMap);
		Declare<int64_t>::Register(typeMap, reflectMap);
		Declare<bool>::Register(typeMap, reflectMap);
		Declare<Void>::Register(typeMap, reflectMap);
		Declare<uint8_t>::Register(typeMap, reflectMap);
		Declare<uint16_t>::Register(typeMap, reflectMap);
		Declare<uint32_t>::Register(typeMap, reflectMap);
		Declare<uint64_t>::Register(typeMap, reflectMap);
		Declare<float>::Register(typeMap, reflectMap);
		Declare<double>::Register(typeMap, reflectMap);
	}
}
