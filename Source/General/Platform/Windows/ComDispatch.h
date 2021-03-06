// ComDispatch.h
// PaintDream (paintdream@paintdream.com)
// 2015-7-4
//

#pragma once
#include "../../../Core/PaintsNow.h"
#include "ComDef.h"

#if !defined(_WIN32) && !defined(WIN32)
#error "ComBridge can only be used on Windows System."
#endif

namespace PaintsNow {
	class ComBridge;
	class ComDispatch : public TReflected<ComDispatch, IReflectObjectComplex> {
	public:
		ComDispatch(IScript::Request& request, ComBridge* bridge, IDispatch* disp);
		~ComDispatch() override;
		virtual void Call(const TProxy<>* p, IScript::Request& request);
		TObject<IReflect>& operator () (IReflect& reflect) override;

		struct Method {
			DISPID id;
			String name;
			String doc;
			IReflect::Param retValue;
			const AutoVariantHandler* retParser;
			std::vector<IReflect::Param> params; // for reflection
			std::vector<const AutoVariantHandler*> parsers; // cache parsers
		};

		IDispatch* GetDispatch() const;

	private:
		IDispatch* dispatch;
		ComBridge* bridge;
		std::vector<Method> methods;
	};
}

