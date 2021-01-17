// RenderableComponent.h
// PaintDream (paintdream@paintdream.com)
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

		/// <summary>
		/// Add render policy of RenderableComponent or its derivations
		/// </summary>
		/// <param name="renderableComponent"> the RenderableComponent </param>
		/// <param name="renderPolicy"> the RenderPolicy </param>
		void RequestAddRenderPolicy(IScript::Request& request, IScript::Delegate<T> renderableComponent, IScript::Delegate<RenderPolicy> renderPolicy) {
			CHECK_REFERENCES_NONE();
			CHECK_DELEGATE(renderableComponent);
			CHECK_DELEGATE(renderPolicy);

			renderableComponent->AddRenderPolicy(renderPolicy.Get());
		}

		/// <summary>
		/// Remove render policy of RenderableComponent or its derivations
		/// </summary>
		/// <param name="renderableComponent"> the RenderableComponent </param>
		/// <param name="renderPolicy"> the RenderPolicy </param>
		void RequestRemoveRenderPolicy(IScript::Request& request, IScript::Delegate<T> renderableComponent, IScript::Delegate<RenderPolicy> renderPolicy) {
			CHECK_REFERENCES_NONE();
			CHECK_DELEGATE(renderableComponent);
			CHECK_DELEGATE(renderPolicy);

			renderableComponent->RemoveRenderPolicy(renderPolicy.Get());
		}

		TObject<IReflect>& operator () (IReflect& reflect) override {
			BaseClass::operator () (reflect);

			if (reflect.IsReflectMethod()) {
				ReflectMethod(RequestAddRenderPolicy)[ScriptMethod = "AddRenderPolicy"];
				ReflectMethod(RequestRemoveRenderPolicy)[ScriptMethod = "RemoveRenderPolicy"];
			}

			return *this;
		}
	};
}
