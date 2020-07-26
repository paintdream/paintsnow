#ifdef _WIN32
#ifdef _DLL
#define _AFXDLL
#endif

#include "../../../Core/PaintsNow.h"
#include "ComDispatch.h"
#include "ComDef.h"
#include "ComBridge.h"

using namespace PaintsNow;

static IReflect::Param GetParamFromDesc(std::unordered_map<size_t, Unique>& typeMap, ITypeInfo* typeInfo, const ELEMDESC& desc) {
	HREFTYPE type;
	String alias;
	size_t vt = GenType(type, alias, typeInfo, desc.tdesc);
	return IReflect::Param(typeMap[vt], alias);
}


ComDispatch::ComDispatch(IScript::Request& request, ComBridge* br, IDispatch* disp) : bridge(br), dispatch(disp) {
	std::unordered_map<Unique, ScriptReflect::Type>& reflectMap = bridge->GetReflectMap();
	std::unordered_map<size_t, Unique>& typeMap = bridge->GetTypeMap();

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
					BSTR bstrName;
					BSTR bstrDoc;
					info->GetDocumentation(func->memid, &bstrName, &bstrDoc, nullptr, nullptr);
					methods.emplace_back(Method());
					Method& method = methods.back();
					method.id = func->memid;
					method.name = ToCoreString(bstrName);
					method.doc = ToCoreString(bstrDoc);

					::SysFreeString(bstrName);
					::SysFreeString(bstrDoc);

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
					static ComVariant bindInterface = L"BindScript";
					if (bstrName == bindInterface) {
						ComVariant param((uint64_t)&request);
						DISPPARAMS args = { &(VARIANT&)param, nullptr, 1, 0 };
						ComVariant var;
						dispatch->Invoke(func->memid, IID_NULL, 0, DISPATCH_METHOD, &args, &(VARIANT&)var, nullptr, nullptr);
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
		std::vector<ComVariant> prepare(size);
		std::vector<AutoVariant*> variants(size);
		request.DoLock();
		for (size_t i = size - 1; (long)i >= 0; i--) {
			// method.parsers[i]->ReadValue(request, &prepare[i]);
			variants[i] = method.parsers[i]->Create(prepare[i]);
			variants[i]->Export(bridge, request);
		}
		request.UnLock();

		ComVariant retValue;
		DISPPARAMS params = { prepare.size() == 0 ? nullptr : &(VARIANT&)prepare[0], nullptr, (UINT)prepare.size(), 0 };
		HRESULT hr;
		if (SUCCEEDED(hr = dispatch->Invoke(method.id, IID_NULL, 0, DISPATCH_METHOD, &params, &(VARIANT&)retValue, nullptr, nullptr))) {
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