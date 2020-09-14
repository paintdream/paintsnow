// TranformComponent.h
// By PaintDream (paintdream@paintdream.com)
// 2018-1-4
//

#pragma once
#include "TransformComponent.h"
#include "../../Module.h"

namespace PaintsNow {
	class TransformComponent;
	class TransformComponentModule : public TReflected<TransformComponentModule, ModuleImpl<TransformComponent> > {
	public:
		TransformComponentModule(Engine& engine);

		TObject<IReflect>& operator () (IReflect& reflect) override;

		TShared<TransformComponent> RequestNew(IScript::Request& request);
		void RequestEditorRotate(IScript::Request& request, IScript::Delegate<TransformComponent> transformComponent, Float2& from, Float2& to);
		void RequestSetRotation(IScript::Request& request, IScript::Delegate<TransformComponent> transformComponent, Float3& rotation);
		Float3 RequestGetRotation(IScript::Request& request, IScript::Delegate<TransformComponent> transformComponent);
		void RequestSetScale(IScript::Request& request, IScript::Delegate<TransformComponent> transformComponent, Float3& scale);
		Float3 RequestGetScale(IScript::Request& request, IScript::Delegate<TransformComponent> transformComponent);
		void RequestSetTranslation(IScript::Request& request, IScript::Delegate<TransformComponent> transformComponent, Float3& translation);
		Float3 RequestGetTranslation(IScript::Request& request, IScript::Delegate<TransformComponent> transformComponent);
		void RequestGetAxises(IScript::Request& request, IScript::Delegate<TransformComponent> transformComponent);
		Float3 RequestGetQuickTranslation(IScript::Request& request, IScript::Delegate<TransformComponent> transformComponent);
		void RequestUpdateTransform(IScript::Request& request, IScript::Delegate<TransformComponent> transformComponent);
	};
}

