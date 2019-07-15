#ifdef _WIN32
#ifdef _DLL
#define _AFXDLL
#endif

#include "../../../Core/PaintsNow.h"
#include <afxwin.h>
#include "ComDispatch.h"
#include "ComDef.h"
#include "ComBridge.h"
#include "../Tunnel.h"

using namespace PaintsNow;
using namespace PaintsNow::NsRayForce;


static IReflect::Param GetParamFromDesc(std::map<size_t, Unique>& typeMap, ITypeInfo* typeInfo, const ELEMDESC& desc) {
	HREFTYPE type;
	String alias;
	size_t vt = GenType(type, alias, typeInfo, desc.tdesc);
	return IReflect::Param(typeMap[vt], alias);
}


ComDispatch::ComDispatch(IScript::Request& request, ComBridge* br, IDispatch* disp) : bridge(br), dispatch(disp) {
	std::map<Unique, ZScriptReflect::Type>& reflectMap = bridge->GetReflectMap();
	std::map<size_t, Unique>& typeMap = bridge->GetTypeMap();

	ITypeInfo* info;
	if (SUCCEEDED(dispatch->GetTypeInfo(0, 0, &info))) {
		// create success
		TYPEATTR* attr;
		if (SUCCEEDED(info->GetTypeAttr(&attr))) {
			// Load funcs
			for (DISPID i = 0; i < attr->cFuncs; i++) {
				FUNCDESC* func = nullptr;
				// Standard methods of IUnknown/IDispatch will hold memids which exceed 0x60000000, skip them
				if (SUCCEEDED(info->GetFuncDesc(i, &func)) && func->memid < 0x10000000) {
					CComBSTR bstrName;
					CComBSTR bstrDoc;
					info->GetDocumentation(func->memid, &bstrName, &bstrDoc, nullptr, nullptr);
					methods.emplace_back(Method());
					Method& method = methods.back();
					method.id = func->memid;
					method.name = ToCoreString(bstrName);
					method.doc = ToCoreString(bstrDoc);
					// Return value
					method.retValue = GetParamFromDesc(typeMap, info, func->elemdescFunc);
					method.retParser = static_cast<const AutoVariantHandler*>(reflectMap[method.retValue.type].parser);

					// Parameters
					for (SHORT i = 0; i < func->cParams; i++) {
						IReflect::Param action = GetParamFromDesc(typeMap, info, func->lprgelemdescParam[i]);
						method.params.emplace_back(action);
						method.parsers.emplace_back(static_cast<const AutoVariantHandler*>(reflectMap[action.type].parser));
					}

					info->ReleaseFuncDesc(func);

					// Call BindScript if necessary
					static CComBSTR bindInterface = L"BindScript";
					if (bstrName == bindInterface) {
						CComVariant param;
						param.ChangeType(VT_I8);
						*(LONGLONG*)&param.lVal = (LONGLONG)&request;
						DISPPARAMS args = { &param, nullptr, 1, 0 };
						CComVariant var;
						dispatch->Invoke(func->memid, IID_NULL, 0, DISPATCH_METHOD, &args, &var, nullptr, nullptr);
					}
				}
			}
		}
		info->ReleaseTypeAttr(attr);
	}
}

TObject<IReflect>& ComDispatch::operator () (IReflect& reflect) {
	for (size_t i = 0; i < methods.size(); i++) {
		const Method& m = methods[i];
		reflect.Method(Unique(), m.name.c_str(), reinterpret_cast<TProxy<>*>(i), m.retValue, m.params, nullptr);
	}
	return *this;
}

void ComDispatch::Call(const TProxy<>* p, IScript::Request& request) {
	// in face p is an integer
	DISPID id = *reinterpret_cast<DISPID*>(&p);
	// fetch correspond method
	const Method& method = methods[id];
	if (dispatch != nullptr) {
		size_t size = method.params.size();
		assert(method.params.size() == method.parsers.size());

		// skip return value
		std::vector<CComVariant> prepare(size);
		std::vector<AutoVariant*> variants(size);
		request.DoLock();
		for (size_t i = size - 1; (long)i >= 0; i--) {
			// method.parsers[i]->ReadValue(request, &prepare[i]);
			variants[i] = method.parsers[i]->Create(prepare[i]);
			variants[i]->Export(bridge, request);
		}
		request.UnLock();

		CComVariant retValue;
		/*
		COleSafeArray arr;
		SAFEARRAYBOUND bound = { 1, 0 };
		arr.Create(VT_I4, 1, &bound);
		LONG i = 0;
		INT val = 4;
		arr.PutElement(&i, &val);
		CComVariant test = arr.Detach();
		DISPPARAMS params = { &test, nullptr, 1, 0 };
		*/
		
		DISPPARAMS params = { prepare.size() == 0 ? nullptr : &prepare[0], nullptr, (UINT)prepare.size(), 0 };
		HRESULT hr;
		if (SUCCEEDED(hr = dispatch->Invoke(method.id, IID_NULL, 0, DISPATCH_METHOD, &params, &retValue, nullptr, nullptr))) {
			// Write returned value
			request.DoLock();
			AutoVariant* retVariant = method.retParser->Create(retValue);
			retVariant->Import(bridge, request);
			delete retVariant;

			for (size_t j = 0; j < size; j++) {
				if (variants[j]->IsOutput()) {
					variants[j]->Import(bridge, request);
				}
			}
			request.UnLock();
		} else {
			request.Error(String("RayForce::ComDispatch error in calling function") + method.name);
		}

		for (size_t j = 0; j < size; j++) {
			variants[j]->Detach(request);
			delete variants[j];
		}
	}
}

IDispatch* ComDispatch::GetDispatch() const {
	return dispatch;
}

ComDispatch::~ComDispatch() {
	dispatch->Release();
}


#endif // _WIN32