// RenderableComponent.h
// By PaintDream (paintdream@paintdream.com)
// 2018-1-19
//

#pragma once
#include "RenderableComponent.h"
#include "../RenderFlow/RenderPolicy.h"
#include "../../Module.h"

namespace PaintsNow {
	class RenderableComponent;
	template <class T>
	class TRenderableComponentModule : public TReflected<TRenderableComponentModule<T>, ModuleImpl<T> > {
	public:
		typedef TReflected<TRenderableComponentModule<T>, ModuleImpl<T> > BaseClass;
		typedef TRenderableComponentModule<T> Class;
		typedef TRenderableComponentModule<T> RenderableComponentModule;
		TRenderableComponentModule(Engine& engine) : BaseClass(engine) {
			// check compatibility
			RenderableComponent* p = (T*)nullptr; (void)p;
		}

		void RequestSetRenderPolicy(IScript::Request& request, IScript::Delegate<T> renderableComponent, IScript::Delegate<RenderPolicy> renderPolicy) {
			CHECK_REFERENCES_NONE();
			CHECK_DELEGATE(renderableComponent);
			CHECK_DELEGATE(renderPolicy);

			renderableComponent->renderPolicy = renderPolicy.Get();
		}

		virtual TObject<IReflect>& operator () (IReflect& reflect) override {
			BaseClass::operator () (reflect);

			if (reflect.IsReflectMethod()) {
				ReflectMethod(RequestSetRenderPolicy)[ScriptMethod = "SetRenderPolicy"];
			}

			return *this;
		}
	};
}
