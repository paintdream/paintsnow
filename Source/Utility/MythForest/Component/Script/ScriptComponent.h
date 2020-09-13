// ScriptComponent.h
// By PaintDream (paintdream@paintdream.com)
// 2015-5-5
//

#pragma once
#include "../../Component.h"

namespace PaintsNow {
	class ScriptComponent : public TAllocatedTiny<ScriptComponent, Component> {
	public:
		enum {
			SCRIPTCOMPONENT_TRANSPARENT = COMPONENT_CUSTOM_BEGIN,
			SCRIPTCOMPONENT_CUSTOM_BEGIN = COMPONENT_CUSTOM_BEGIN << 1
		};

		ScriptComponent(const String& name);
		virtual ~ScriptComponent();
		void SetHandler(IScript::Request& request, Event::EVENT_ID event, IScript::Request::Ref handler);
		virtual Tiny::FLAG GetEntityFlagMask() const override;
		virtual void ScriptUninitialize(IScript::Request& request) override;
		virtual const String& GetAliasedTypeName() const override;

	protected:
		virtual void DispatchEvent(Event& event, Entity* entity) override;
		void UpdateEntityFlagMask();

	protected:
		String name;
		Tiny::FLAG entityFlagMask;
		IScript::Request::Ref handlers[Event::EVENT_END];
	};
}

