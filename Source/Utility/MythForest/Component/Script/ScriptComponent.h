// ScriptComponent.h
// By PaintDream (paintdream@paintdream.com)
// 2015-5-5
//

#ifndef __SCRIPTCOMPONENT_H__
#define __SCRIPTCOMPONENT_H__

#include "../../Component.h"

namespace PaintsNow {
	namespace NsMythForest {
		class ScriptComponent : public TAllocatedTiny<ScriptComponent, Component> {
		public:
			enum {
				SCRIPTCOMPONENT_TRANSPARENT = COMPONENT_CUSTOM_BEGIN,
				SCRIPTCOMPONENT_CUSTOM_BEGIN = COMPONENT_CUSTOM_BEGIN << 1
			};

			ScriptComponent();
			virtual ~ScriptComponent();
			void SetHandler(IScript::Request& request, Event::EVENT_ID event, IScript::Request::Ref handler);
			virtual Tiny::FLAG GetEntityFlagMask() const override;
			virtual void Uninitialize(Engine& engine, Entity* entity) override;

		protected:
			virtual void DispatchEvent(Event& event, Entity* entity) override;

		private:
			IScript::Request::Ref handlers[Event::EVENT_END];
		};
	}
}


#endif // __SCRIPTCOMPONENT_H__