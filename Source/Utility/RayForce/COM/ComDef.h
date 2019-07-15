// ComDef.h
// By PaintDream (paintdream@paintdream.com)
// 2015-7-4
//

#ifndef __COMDEF_H__
#define __COMDEF_H__

#include "../../../Core/PaintsNow.h"

#if defined(_WIN32)

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
#include <afxdisp.h>
#endif
#include <atlbase.h>

#if defined(_MSC_VER) && _MSC_VER <= 1200
#undef typename
#else

#include <atlsafe.h>
#endif

#include "../../../Core/Interface/IType.h"
#include "../../../General/Misc/ZScriptReflect.h"
#include <string>
#include <map>

namespace PaintsNow {
	namespace NsRayForce {
		class ComBridge;
		String ToCoreString(const BSTR& str);
		CComBSTR FromCoreString(const String& str);
		size_t GenType(HREFTYPE& refType, String& userDefinedName, ITypeInfo* typeInfo, const TYPEDESC& desc);
		void InitComReflectMap(std::map<size_t, Unique>& typeMap, std::map<Unique, ZScriptReflect::Type>& reflectMap);
		void InitMshtmlView();
		void InitDPIAware();

		class AutoVariant {
		public:
			virtual ~AutoVariant();
			virtual bool IsOutput() const = 0;
			virtual void Export(ComBridge* bridge, IScript::Request& request) = 0;
			virtual void Import(ComBridge* bridge, IScript::Request& request) = 0;
			virtual void Detach(IScript::Request& request) = 0;
		};

		class AutoVariantHandler : public ZScriptReflect::ValueParserBase {
		public:
			virtual AutoVariant* Create(CComVariant& t) const = 0;
		};


		template <class Impl, class Base>
		struct CComPtrInternalCreator {
			CComPtrInternalCreator(CComModule& m) : module(m) {}
			inline Base* operator () () {
				Base* ptrType;
				IClassFactory* classFactory;
				if (SUCCEEDED(module.GetClassObject(__uuidof(Impl), IID_IClassFactory, (void**)&classFactory))) {
					if (SUCCEEDED(classFactory->CreateInstance(nullptr, __uuidof(Base), (void**)&ptrType))) {
						classFactory->Release();
						return ptrType;
					}
					
					classFactory->Release();
				}

				return nullptr;
			}

			CComModule& module;
		};
	}
}

#endif

#endif // __COMDEF_H__