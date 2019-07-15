#ifdef _WIN32

#include "ComDef.h"
#include "../../../General/Template/TMaskType.h"
#include "../../../Core/Interface/IScript.h"
#include "../Tunnel.h"
#include "ComDispatch.h"
#include "ComBridge.h"

#pragma warning(disable:4996)

namespace PaintsNow {
	namespace NsRayForce {
		String ToCoreString(const BSTR& str) {
			if (str != nullptr) {
				const char* p = (const char*)(str);
				String w(p, ::SysStringByteLen(str));
				return SystemToUtf8(w);
			} else {
				return "";
			}
		}

		CComBSTR FromCoreString(const String& str) {
			String w = Utf8ToSystem(str);
			return CComBSTR((const OLECHAR*)w.data());
		}
	}
}

using namespace PaintsNow;
using namespace PaintsNow::NsRayForce;

AutoVariant::~AutoVariant() {}

typedef VARIANT Variant;
class Uint {
public:
	Uint(UINT u = 0) : value(u) {}
	operator UINT () const {
		return value;
	}

	UINT value;
};

class Int {
public:
	Int(INT u = 0) : value(u) {}
	operator INT () const {
		return value;
	}

	INT value;
};

class UnknownType {
public:
	UnknownType(LPUNKNOWN p = nullptr) : pointer(p) {}
	LPUNKNOWN pointer;
};

class UserObject {
public:
	UserObject(ComBridge* br = nullptr, LPUNKNOWN p = nullptr) : bridge(br), pointer(p) {}
	~UserObject() {
		if (pointer != nullptr) {
			pointer->Release();
		}
	}
	LPUNKNOWN pointer;
	ComBridge* bridge;
};

class Reference {
public:
	IScript::Request::Ref ref;
};

namespace PaintsNow {
	IScript::Request& operator << (IScript::Request& request, const UnknownType&) { return request; }
	IScript::Request& operator >> (IScript::Request& request, UnknownType&) { return request; }
	IScript::Request& operator << (IScript::Request& request, const UserObject& object) {
		assert(object.bridge != nullptr);
		IUnknown* ptr = object.pointer;
		IDispatch* disp = (IDispatch*)object.pointer;
		if (ptr != nullptr) {
			IDispatch* disp;
			if (SUCCEEDED(ptr->QueryInterface(IID_IDispatch, (LPVOID*)&disp))) {
				ComDispatch* d = new ComDispatch(request, object.bridge, disp);
				TShared<Tunnel> tunnel = TShared<Tunnel>::From(new Tunnel(object.bridge, d));
				request << tunnel;
				disp->Release();
			} else {
				request << nil;
			}
		} else {
			request << nil;
		}
		return request;
	}

	IScript::Request& operator >> (IScript::Request& request, UserObject& object) {
		assert(object.bridge != nullptr);
		IScript::Delegate<Tunnel> tunnel;
		request >> tunnel;
		if (tunnel) {
			IDispatch* disp = (static_cast<ComDispatch*>(tunnel->GetHost()))->GetDispatch();
			disp->QueryInterface(IID_IUnknown, (LPVOID*)&object.pointer);
		} else {
			object.pointer = nullptr;
		}
		return request;
	}

	IScript::Request& operator << (IScript::Request& request, const Variant&) { 
		return request;
	}

	IScript::Request& operator >> (IScript::Request& request, Variant&) { return request; }

	IScript::Request& operator << (IScript::Request& request, const Int& v) {
		request << v.value;
		return request;
	}

	IScript::Request& operator >> (IScript::Request& request, Int& v) {
		request >> v.value;
		return request;
	}

	IScript::Request& operator << (IScript::Request& request, const Uint& v) {
		request << v.value;
		return request;
	}

	IScript::Request& operator >> (IScript::Request& request, Uint& v) {
		request >> v.value;
		return request;
	}
}

template <class T>
struct MapType {
	enum { VtType = VT_PTR, VtTypeTarget = VtType };
};

/*
template <>
struct MapType<IScript::Request::Ref> {
	typedef void* target;
	enum { VtType = VT_PTR, VtTypeTarget = VtType };

	static CComVariant From(const IScript::Request::Ref& ref) {
		return CComVariant((LPVOID)ref.value);
	}

	static IScript::Request::Ref To(ComBridge* br, const CComVariant& str) {
		return IScript::Request::Ref((long)str.pulVal);
	}
};*/


template <>
struct MapType<String> {
	typedef BSTR target;
	enum { VtType = VT_BSTR, VtTypeTarget = VtType };
	static CComVariant From(const String& str) {
		return FromCoreString(str);
	}

	static String To(ComBridge* br, const CComVariant& str) {
		if (str.vt == VT_EMPTY)
			return "";
		else
			return ToCoreString(str.bstrVal);
	}
};

template <>
struct MapType<int8_t> {
	typedef CHAR target;
	enum { VtType = VT_I1, VtTypeTarget = VtType };
	static CComVariant From(const int8_t& s) {
		return s;
	}

	static int8_t To(ComBridge* br, const CComVariant& str) {
		return str.cVal;
	}
};

template <>
struct MapType<int16_t> {
	typedef SHORT target;
	enum { VtType = VT_I2, VtTypeTarget = VtType };
	static CComVariant From(const int16_t& s) {
		return s;
	}

	static int16_t To(ComBridge* br, const CComVariant& str) {
		return str.iVal;
	}
};

template <>
struct MapType<int32_t> {
	typedef INT target;
	enum { VtType = VT_I4, VtTypeTarget = VtType };
	static CComVariant From(const int32_t& s) {
		return s;
	}

	static int16_t To(ComBridge* br, const CComVariant& str) {
		return str.intVal;
	}
};

template <>
struct MapType<int64_t> {
	typedef LONGLONG target;
	enum { VtType = VT_I8, VtTypeTarget = VtType };
	static CComVariant From(const int64_t& s) {
		CComVariant v;
		v.ChangeType(VT_I8);
		v.llVal = s;
		return v;
	}

	static int64_t To(ComBridge* br, const CComVariant& str) {
		return str.llVal;
	}
};

template <>
struct MapType<long> {
	typedef LONG target;
	enum { VtType = VT_I4, VtTypeTarget = VtType }; // x64 ?!
	static CComVariant From(const long& s) {
		return s;
	}

	static int16_t To(ComBridge* br, const CComVariant& str) {
		return str.intVal;
	}
};

template <>
struct MapType<uint8_t> {
	typedef UCHAR target;
	enum { VtType = VT_UI1, VtTypeTarget = VtType };
	static CComVariant From(const uint8_t& s) {
		return s;
	}

	static uint8_t To(ComBridge* br, const CComVariant& str) {
		return str.bVal;
	}
};

template <>
struct MapType<uint16_t> {
	typedef USHORT target;
	enum { VtType = VT_UI2, VtTypeTarget = VtType };
	static CComVariant From(const uint16_t& s) {
		return s;
	}

	static int16_t To(ComBridge* br, const CComVariant& str) {
		return str.uiVal;
	}
};

template <>
struct MapType<uint32_t> {
	typedef UINT target;
	enum { VtType = VT_UI4, VtTypeTarget = VtType };
	static CComVariant From(const uint32_t& s) {
		CComVariant v;
		v.ChangeType(VT_UI4);
		v.uintVal = s;
		return v;
	}

	static int32_t To(ComBridge* br, const CComVariant& str) {
		return str.uintVal;
	}
};

template <>
struct MapType<uint64_t> {
	typedef LONGLONG target;
	enum { VtType = VT_UI8, VtTypeTarget = VtType };
	static CComVariant From(const uint64_t& s) {
		CComVariant v;
		v.ChangeType(VT_UI8);
		v.llVal = s;
		return v;
	}

	static int64_t To(ComBridge* br, const CComVariant& str) {
		return str.ullVal;
	}
};

template <>
struct MapType<bool> {
	typedef BOOL target;
	enum { VtType = VT_BOOL, VtTypeTarget = VtType };
	static CComVariant From(const bool& s) {
		return (VARIANT_BOOL)s;
	}

	static bool To(ComBridge* br, const CComVariant& str) {
		return str.boolVal != 0;
	}
};

template <>
struct MapType<float> {
	typedef FLOAT target;
	enum { VtType = VT_R4, VtTypeTarget = VtType };
	static CComVariant From(const float& s) {
		return s;
	}

	static float To(ComBridge* br, const CComVariant& str) {
		return str.fltVal;
	}
};

template <>
struct MapType<double> {
	typedef DOUBLE target;
	enum { VtType = VT_R8, VtTypeTarget = VtType };
	static CComVariant From(const double& s) {
		return s;
	}

	static DOUBLE To(ComBridge* br, const CComVariant& str) {
		return str.dblVal;
	}
};

template <>
struct MapType<Int> {
	typedef INT target;
	enum { VtType = VT_INT, VtTypeTarget = VT_I4 };
	static CComVariant From(const Int& s) {
		return CComVariant(s, VT_I4); // must pass I4 instead of INT, don't know why
	}

	static Int To(ComBridge* br, const CComVariant& str) {
		return str.intVal;
	}
};

template <>
struct MapType<Uint> {
	typedef UINT target;
	enum { VtType = VT_UINT, VtTypeTarget = VT_UI4 };
	static CComVariant From(const Uint& s) {
		return CComVariant((UINT)s, VT_UI4);
	}

	static Uint To(ComBridge* br, const CComVariant& str) {
		return str.intVal;
	}
};

template <>
struct MapType<Void> {
	typedef Void target;
	enum { VtType = VT_VOID, VtTypeTarget = VtType };
	static CComVariant From(const Void& s) {
		return CComVariant();
	}

	static Void To(ComBridge* br, const CComVariant& str) {
		return Void();
	}
};

template <>
struct MapType<UnknownType> {
	typedef IUnknown* target;
	enum { VtType = VT_UNKNOWN, VtTypeTarget = VtType };
	static CComVariant From(const UnknownType& s) {
		return s.pointer;
	}

	static UnknownType To(ComBridge* br, const CComVariant& str) {
		return str.punkVal;
	}
};

template <>
struct MapType<UserObject> {
	typedef LPUNKNOWN target;
	enum { VtType = VT_USERDEFINED, VtTypeTarget = VtType };
	static CComVariant From(UserObject& s) {
		return CComVariant(s.pointer);
	}

	static UserObject To(ComBridge* br, const CComVariant& str) {
		return UserObject(br, str.punkVal);
	}
};

template <>
struct MapType<Variant> {
	typedef VARIANT target;
	enum { VtType = VT_VARIANT, VtTypeTarget = VtType };
	static CComVariant From(const Variant& s) {
		return s;
	}

	static Variant To(ComBridge* br, const CComVariant& str) {
		return str;
	}
};



enum { CAST_ARRAY = 1, CAST_REF = 2, CAST_SELF = 3, CAST_SIZE = 4 };

BEGIN_TYPEMASK(TMaskCast, CAST_SIZE)

static CComVariant dummy;

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
		CComVariant& var;
		Impl(CComVariant& v = dummy) : var(v) {}

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
			var = CComVariant(MapType<T>::From(self));
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
		CComVariant& var;
		IScript::Request::Ref handle;
		Impl(CComVariant& v = dummy) : var(v) {
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
			
				var.vt = self.var.vt | VT_BYREF;
				var.pparray = &self.var.parray;
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
		CComVariant& var;
		Impl(CComVariant& v = dummy) : var(v) {}

		virtual bool IsOutput() const {
			return false;
		}

		virtual void Detach(IScript::Request& request) {}
		virtual void Export(ComBridge* bridge, IScript::Request& request) {
			COleSafeArray safeArray;
			IScript::Request::TableStart ts;
			request >> ts;
			SAFEARRAYBOUND bound = { (ULONG)ts.count, 0 };
			safeArray.Create(T::CastType == CAST_ARRAY ? VT_ARRAY : T::CastType == CAST_REF ? VT_PTR : MapType<T::type>::VtTypeTarget, 1, &bound);
			container.clear();
			for (size_t i = 0; i < ts.count; i++) {
				long index = (long)i;
				container.emplace_back(T());
				container.back().Export(bridge, request);
				safeArray.PutElement(&index, &container.back().var.intVal);
			}

			request >> endtable;

			var = safeArray.Detach();
		}

		virtual void Import(ComBridge* br, IScript::Request& request) {
			COleSafeArray safeArray;
			safeArray.Attach(var);
			LONG lower;
			safeArray.GetLBound(1, &lower);
			LONG upper;
			safeArray.GetUBound(1, &upper);
			request << begintable;
			std::list<T>::iterator it = container.begin();
			for (LONG index = lower; index <= upper; index++) {
				safeArray.GetElement(&index, &(*it).var.intVal);
				it->Import(br, request);
				++it;
			}
			request << endtable;
			safeArray.Detach();
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

	virtual AutoVariant* Create(CComVariant& variant) const {
		return new type(variant);
	}

	void WriteValue(IScript::Request& request, const void* base) const {
		// Not implemented
		assert(false);
		// assume base is an object of type CComVariant
		// type::Write(request, var, *reinterpret_cast<CComVariant*>(const_cast<void*>(base)));
	}

	void ReadValue(IScript::Request& request, void* base) const {
		// Not implemented
		assert(false);
		// type::Read(request, var, *reinterpret_cast<CComVariant*>(base));
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
			static void Register(std::map<size_t, Unique>& typeMap, std::map<Unique, ZScriptReflect::Type>& reflectMap, const String& alias) {
				typedef typename ActionBase<T, base> Action;
				typedef typename Action::type Type;
				static Action action;
				Unique typeID = UniqueType<Action::type>::Get();
				reflectMap[typeID] = ZScriptReflect::Type(Type::GetName(alias), &action);
				typeMap[type] = typeID;
				// printf("[%8X] To [%8X] = %s\n", type, typeID, Type::GetName(alias).c_str());

				// Next generation
				RegType<level - 1>::Based<(base * CAST_SIZE + CAST_ARRAY)>::Reg<T, ADD_MASK(type, CAST_ARRAY)>::Register(typeMap, reflectMap, alias);
				RegType<level - 1>::Based<(base * CAST_SIZE + CAST_REF)>::Reg<T, ADD_MASK(type, CAST_REF)>::Register(typeMap, reflectMap, alias);
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
			static void Register(std::map<size_t, Unique>& typeMap, std::map<Unique, ZScriptReflect::Type>& reflectMap, const String& alias) {}
		};
	};
};


template <class T>
struct Declare {
	static void Register(std::map<size_t, Unique>& typeMap, std::map<Unique, ZScriptReflect::Type>& reflectMap) {
		RegType<3>::Based<CAST_SELF>::Reg<T, MapType<T>::VtType>::Register(typeMap, reflectMap, reflectMap[UniqueType<T>::Get()].name);
	}
};

namespace PaintsNow {
	namespace NsRayForce {
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
					CComBSTR bstrName;
					/*
					TYPEATTR* attr;
					if (SUCCEEDED(w->GetTypeAttr(&attr))) {
					GUID guid = attr->guid;
					w->ReleaseTypeAttr(attr);
					}*/
					w->GetDocumentation(MEMBERID_NIL, &bstrName, nullptr, nullptr, nullptr);
					userDefinedName = ToCoreString(bstrName);
					refType = h;
					w->Release();
				}

				return vt;
			} else {
				return vt;
			}
		}

		void InitDPIAware() {
			typedef BOOL(WINAPI* SetProcessDPIAware)();
			SetProcessDPIAware proc = (SetProcessDPIAware)::GetProcAddress(::GetModuleHandle(_T("user32.dll")), "SetProcessDPIAware");
			if (proc != nullptr) {
				proc();
			}
		}

		void InitMshtmlView() {
			TCHAR fullPath[MAX_PATH * 2];
			::GetModuleFileName(nullptr, fullPath, MAX_PATH * 2);
			CString path = fullPath;
			int pos = path.ReverseFind(_T('\\'));
			CString name = path.Right(path.GetLength() - pos - 1);
			TRACE(name);

			// Query newest version
			CRegKey reg;
			if (reg.Open(HKEY_LOCAL_MACHINE, _T("SOFTWARE\\Microsoft\\Internet Explorer")) == ERROR_SUCCESS) {
				DWORD sizeSvc = MAX_PATH, sizeVersion = MAX_PATH;
				TCHAR value[MAX_PATH];
				DWORD target = 0;
				if (reg.QueryValue(value, _T("svcVersion"), &sizeSvc) == ERROR_SUCCESS || reg.QueryValue(value, _T("Version"), &sizeVersion) == ERROR_SUCCESS) {
					CString version = value;
					if (version.Find(_T("7.")) == 0) {
						target = 7000;
					}
					else if (version.Find(_T("8.")) == 0) {
						target = 8000;
					}
					else if (version.Find(_T("9.")) == 0) {
						target = 9000;
					}
					else if (version.Find(_T("10.")) == 0) {
						target = 10000;
					}
					else if (version.Find(_T("11.")) == 0) {
						target = 11001;
					}

					if (target != 0) {
						OSVERSIONINFO oi;
						oi.dwOSVersionInfoSize = sizeof(oi);
						GetVersionEx(&oi);
						if (oi.dwMajorVersion >= 6) // Vista or later, set uac
						{
							SHELLEXECUTEINFO shExInfo = { 0 };
							shExInfo.cbSize = sizeof(SHELLEXECUTEINFO);
							shExInfo.hwnd = nullptr;
							shExInfo.lpVerb = _T("runas");
							shExInfo.lpFile = _T("reg.exe");

							if (reg.Open(HKEY_LOCAL_MACHINE, _T("SOFTWARE\\Microsoft\\Internet Explorer\\Main\\FeatureControl\\FEATURE_BROWSER_EMULATION")) == ERROR_SUCCESS) {
								DWORD v = 0;
								if (reg.QueryValue(v, (LPCTSTR)name) != ERROR_SUCCESS || v != target) {
									CString command;
									command.Format(_T("add \"HKLM\\SOFTWARE\\Microsoft\\Internet Explorer\\Main\\FeatureControl\\FEATURE_BROWSER_EMULATION\" /v \"%s\" /t REG_DWORD /d %lu"), (LPCTSTR)name, target);
									shExInfo.lpParameters = (LPCTSTR)command;
									if (!ShellExecuteEx(&shExInfo)) {
										AfxMessageBox(_T("Register IE Emulation failed!"));
									}
								}
							}

							if (reg.Open(HKEY_LOCAL_MACHINE, _T("SOFTWARE\\Wow6432Node\\Microsoft\\Internet Explorer\\Main\\FeatureControl\\FEATURE_BROWSER_EMULATION")) == ERROR_SUCCESS) {
								DWORD v = 0;
								if (reg.QueryValue(v, (LPCTSTR)name) != ERROR_SUCCESS || v != target) {
									CString command;
									command.Format(_T("add \"HKLM\\SOFTWARE\\Wow6432Node\\Microsoft\\Internet Explorer\\Main\\FeatureControl\\FEATURE_BROWSER_EMULATION\" /v \"%s\" /t REG_DWORD /d %lu"), (LPCTSTR)name, target);
									shExInfo.lpParameters = (LPCTSTR)command;
									if (!ShellExecuteEx(&shExInfo)) {
										AfxMessageBox(_T("Register IE Emulation failed!"));
									}
								}
							}
						} else {
							if (reg.Open(HKEY_LOCAL_MACHINE, _T("SOFTWARE\\Microsoft\\Internet Explorer\\Main\\FeatureControl\\FEATURE_BROWSER_EMULATION")) == ERROR_SUCCESS) {
								reg.SetValue(target, name);
							}

							if (reg.Open(HKEY_LOCAL_MACHINE, _T("SOFTWARE\\Wow6432Node\\Microsoft\\Internet Explorer\\Main\\FeatureControl\\FEATURE_BROWSER_EMULATION")) == ERROR_SUCCESS) {
								reg.SetValue(target, name);
							}
						}
					}
				}
			}
		}


		void InitComReflectMap(std::map<size_t, Unique>& typeMap, std::map<Unique, ZScriptReflect::Type>& reflectMap) {
			reflectMap = ZScriptReflect::GetGlobalMap();
			reflectMap[UniqueType<UserObject>::Get()] = ZScriptReflect::Type(String("<Object>"), static_cast<ZScriptReflect::ValueParserBase*>(nullptr));
			reflectMap[UniqueType<Variant>::Get()] = ZScriptReflect::Type(String("Variant"), static_cast<ZScriptReflect::ValueParserBase*>(nullptr));
			reflectMap[UniqueType<UserObject>::Get()] = ZScriptReflect::Type(String("UserObject"), static_cast<ZScriptReflect::ValueParserBase*>(nullptr));
			reflectMap[UniqueType<Uint>::Get()] = ZScriptReflect::Type(String("UINT"), static_cast<ZScriptReflect::ValueParserBase*>(nullptr));
			reflectMap[UniqueType<Int>::Get()] = ZScriptReflect::Type(String("INT"), static_cast<ZScriptReflect::ValueParserBase*>(nullptr));

			// Declare<IScript::Request::Ref>::Register(typeMap, reflectMap);
			Declare<String>::Register(typeMap, reflectMap);
			Declare<int8_t>::Register(typeMap, reflectMap);
			Declare<int16_t>::Register(typeMap, reflectMap);
			Declare<int32_t>::Register(typeMap, reflectMap);
			Declare<int64_t>::Register(typeMap, reflectMap);
			Declare<Int>::Register(typeMap, reflectMap);
			Declare<Uint>::Register(typeMap, reflectMap);
			Declare<bool>::Register(typeMap, reflectMap);
			Declare<UnknownType>::Register(typeMap, reflectMap);
			Declare<Void>::Register(typeMap, reflectMap);
			Declare<UserObject>::Register(typeMap, reflectMap);
			Declare<Variant>::Register(typeMap, reflectMap);

			Declare<uint8_t>::Register(typeMap, reflectMap);
			Declare<uint16_t>::Register(typeMap, reflectMap);
			Declare<uint32_t>::Register(typeMap, reflectMap);
			Declare<uint64_t>::Register(typeMap, reflectMap);
			Declare<float>::Register(typeMap, reflectMap);
			Declare<double>::Register(typeMap, reflectMap);
		}
	}
}


#endif // _WIN32