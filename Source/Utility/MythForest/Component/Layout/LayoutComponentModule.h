// LayoutComponentModule.h
// By PaintDream (paintdream@paintdream.com)
// 2015-5-5
//

#pragma once
#include "LayoutComponent.h"
#include "../../Module.h"

namespace PaintsNow {
	class Entity;
	class LayoutComponent;
	class LayoutComponentModule : public TReflected<LayoutComponentModule, ModuleImpl<LayoutComponent> > {
	public:
		LayoutComponentModule(Engine& engine);
		virtual ~LayoutComponentModule();
		virtual TObject<IReflect>& operator () (IReflect& reflect) override;

		TShared<LayoutComponent> RequestNew(IScript::Request& request);
		Float2 RequestGetScrollSize(IScript::Request& request, IScript::Delegate<LayoutComponent> layoutComponent);
		Float2 RequestGetScrollOffset(IScript::Request& request, IScript::Delegate<LayoutComponent> layoutComponent);
		void RequestSetScrollOffset(IScript::Request& request, IScript::Delegate<LayoutComponent> layoutComponent, Float2& position);
		void RequestSetLayout(IScript::Request& request, IScript::Delegate<LayoutComponent> layoutComponent, const String& layout);
		void RequestSetWeight(IScript::Request& request, IScript::Delegate<LayoutComponent> layoutComponent, int64_t weight);
		int32_t RequestGetWeight(IScript::Request& request, IScript::Delegate<LayoutComponent> layoutComponent);
		void RequestSetRect(IScript::Request& request, IScript::Delegate<LayoutComponent> layoutComponent, Float4& rect);
		Float4 RequestGetRect(IScript::Request& request, IScript::Delegate<LayoutComponent> layoutComponent);
		Float4 RequestGetClippedRect(IScript::Request& request, IScript::Delegate<LayoutComponent> layoutComponent);
		void RequestSetSize(IScript::Request& request, IScript::Delegate<LayoutComponent> layoutComponent, Float4& size);
		Float4 RequestGetSize(IScript::Request& request, IScript::Delegate<LayoutComponent> layoutComponent);
		void RequestSetPadding(IScript::Request& request, IScript::Delegate<LayoutComponent> layoutComponent, Float4& padding);
		Float4 RequestGetPadding(IScript::Request& request, IScript::Delegate<LayoutComponent> layoutComponent);
		void RequestSetMargin(IScript::Request& request, IScript::Delegate<LayoutComponent> layoutComponent, Float4& margin);
		Float4 RequestGetMargin(IScript::Request& request, IScript::Delegate<LayoutComponent> layoutComponent);
		void RequestSetFitContent(IScript::Request& request, IScript::Delegate<LayoutComponent> window, bool fitContent);
		bool RequestGetFitContent(IScript::Request& request, IScript::Delegate<LayoutComponent> window);
		void RequestSetIndexRange(IScript::Request& request, IScript::Delegate<LayoutComponent> layoutComponent, int start, int count);
	};
}

