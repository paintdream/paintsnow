// ComDef.h
// By PaintDream (paintdream@paintdream.com)
// 2015-7-4
//

#ifndef __COMDEF_H__
#define __COMDEF_H__

#include "../../../Core/PaintsNow.h"

#if defined(_MSC_VER) && _MSC_VER <= 1200
#include <afxdisp.h>
#ifndef typename
#define typename class
#endif
#else
#ifdef _MBCS
#undef _MBCS
#define UNICODE
#define _UNICODE
#endif

#ifdef _DLL
#define _AFXDLL
#endif
#include <combaseapi.h>
#endif

#if defined(_MSC_VER) && _MSC_VER <= 1200
#undef typename
#else
#endif

#include "../../../Core/Interface/IReflect.h"
#include "../../../Core/Interface/IType.h"
#include "../../Misc/ScriptReflect.h"

namespace PaintsNow {
	class ComBridge;
	class ComVariant {
	public:
		ComVariant() { VariantInit(&var); }
		ComVariant(const VARIANT& v) { VariantCopy(&var, (VARIANT*)&v); }
		~ComVariant() {
			VariantClear(&var);
		}

		void Clear() {
			VariantClear(&var);
			VariantInit(&var);
		}

		ComVariant(const ComVariant& rhs) {
			VariantCopy(&var, (VARIANT*)&rhs.var);
		}

		ComVariant(rvalue<ComVariant> rhs) {
			Clear();
			ComVariant& v = rhs;
			std::swap(var, v.var);
		}

		ComVariant& operator = (const ComVariant& rhs) {
			Clear();
			VariantCopy(&var, (VARIANT*)&rhs.var);

			return *this;
		}

		operator VARIANT& () {
			return var;
		}

		VARIANT* operator -> () {
			return &var;
		}

		const VARIANT* operator -> () const {
			return &var;
		}

		ComVariant(LPDISPATCH val) {
			var.vt = VT_UNKNOWN;
			var.pdispVal = val;
			val->AddRef();
		}

		ComVariant(BSTR bstr) {
			var.vt = VT_BSTR;
			var.bstrVal = ::SysAllocString(bstr);
		}

		operator BSTR() {
			assert(var.vt == VT_BSTR);
			return var.bstrVal;
		}

		ComVariant(bool val) {
			var.vt = VT_BOOL;
			var.boolVal = val ? VARIANT_TRUE : VARIANT_FALSE;
		}

		ComVariant(int8_t val) {
			var.vt = VT_I1;
			var.cVal = val;
		}

		ComVariant(uint8_t val) {
			var.vt = VT_UI1;
			var.bVal = val;
		}

		ComVariant(int16_t val) {
			var.vt = VT_I2;
			var.iVal = val;
		}

		ComVariant(uint16_t val) {
			var.vt = VT_UI2;
			var.uiVal = val;
		}

		ComVariant(int32_t val) {
			var.vt = VT_I4;
			var.intVal = val;
		}

		ComVariant(uint32_t val) {
			var.vt = VT_UI4;
			var.uintVal = val;
		}

		ComVariant(int64_t val) {
			var.vt = VT_I8;
			var.llVal = val;
		}

		ComVariant(uint64_t val) {
			var.vt = VT_UI8;
			var.ullVal = val;
		}

		ComVariant(float val) {
			var.vt = VT_R4;
			var.fltVal = val;
		}

		ComVariant(double val) {
			var.vt = VT_R8;
			var.dblVal = val;
		}

	protected:
		VARIANT var;
	};

	String ToCoreString(const BSTR& str);
	ComVariant FromCoreString(const String& str);
	size_t GenType(HREFTYPE& refType, String& userDefinedName, ITypeInfo* typeInfo, const TYPEDESC& desc);
	void InitComReflectMap(std::unordered_map<size_t, Unique>& typeMap, std::unordered_map<Unique, ScriptReflect::Type>& reflectMap);
	void InitDPIAware();

	class AutoVariant {
	public:
		virtual ~AutoVariant();
		virtual bool IsOutput() const = 0;
		virtual void Export(ComBridge* bridge, IScript::Request& request) = 0;
		virtual void Import(ComBridge* bridge, IScript::Request& request) = 0;
		virtual void Detach(IScript::Request& request) = 0;
	};

	class AutoVariantHandler : public ScriptReflect::ValueParserBase {
	public:
		virtual AutoVariant* Create(ComVariant& t) const = 0;
	};
}

#endif // __COMDEF_H__