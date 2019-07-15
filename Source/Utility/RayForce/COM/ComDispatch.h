// ComDispatch.h
// By PaintDream (paintdream@paintdream.com)
// 2015-7-4
//

#ifndef __COMDISPATCH_H__
#define __COMDISPATCH_H__

#include "../../../Core/PaintsNow.h"
#include "../../../Core/Interface/IReflect.h"
#include "../../../Core/Interface/IScript.h"
#include "../../../General/Misc/ZScriptReflect.h"

#if !defined(_WIN32) && !defined(WIN32)
#error "ComBridge can only be used on Windows System."
#endif

#include "ComDef.h"

namespace PaintsNow {
	namespace NsRayForce {
		class ComBridge;
		class ComDispatch : public TReflected<ComDispatch, IReflectObjectComplex> {
		public:
			ComDispatch(IScript::Request& request, ComBridge* bridge, IDispatch* disp);
			virtual ~ComDispatch();
			virtual void Call(const TProxy<>* p, IScript::Request& request);
			virtual TObject<IReflect>& operator () (IReflect& reflect) override;

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
}

#endif // __COMDISPATCH_H__