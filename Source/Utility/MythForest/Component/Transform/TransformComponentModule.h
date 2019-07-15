// TranformComponent.h
// By PaintDream (paintdream@paintdream.com)
// 2018-1-4
//

#ifndef __TRANSFORMCOMPONENTMODULE_H__
#define __TRANSFORMCOMPOENNTMODULE_H__

#include "TransformComponent.h"
#include "../../Module.h"

namespace PaintsNow {
	namespace NsMythForest {
		class TransformComponent;
		class TransformComponentModule  : public TReflected<TransformComponentModule , ModuleImpl<TransformComponent> > {
		public:
			TransformComponentModule(Engine& engine);

			virtual TObject<IReflect>& operator () (IReflect& reflect) override;

			void RequestNew(IScript::Request& request);
			void RequestEditorRotate(IScript::Request& request, IScript::Delegate<TransformComponent> transformComponent, Float2& from, Float2& to);
			void RequestSetRotation(IScript::Request& request, IScript::Delegate<TransformComponent> transformComponent, Float3& rotation);
			void RequestGetRotation(IScript::Request& request, IScript::Delegate<TransformComponent> transformComponent);
			void RequestSetScale(IScript::Request& request, IScript::Delegate<TransformComponent> transformComponent, Float3& scale);
			void RequestGetScale(IScript::Request& request, IScript::Delegate<TransformComponent> transformComponent);
			void RequestSetTranslation(IScript::Request& request, IScript::Delegate<TransformComponent> transformComponent, Float3& translation);
			void RequestGetTranslation(IScript::Request& request, IScript::Delegate<TransformComponent> transformComponent);
			void RequestGetAxises(IScript::Request& request, IScript::Delegate<TransformComponent> transformComponent);
			void RequestGetQuickTranslation(IScript::Request& request, IScript::Delegate<TransformComponent> transformComponent);
			void RequestUpdateTransform(IScript::Request& request, IScript::Delegate<TransformComponent> transformComponent);
		};
	}
}


#endif // __TRANSFORMCOMPONENTMODULE_H__