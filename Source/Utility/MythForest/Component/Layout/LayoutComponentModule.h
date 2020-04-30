// LayoutComponentModule.h
// By PaintDream (paintdream@paintdream.com)
// 2015-5-5
//

#ifndef __LAYOUTCOMPONENTMODULE_H__
#define __LAYOUTCOMPONENTMODULE_H__

#include "LayoutComponent.h"
#include "../../Module.h"

namespace PaintsNow {
	namespace NsMythForest {
		class Entity;
		class LayoutComponent;
		class LayoutComponentModule  : public TReflected<LayoutComponentModule, ModuleImpl<LayoutComponent> > {
		public:
			LayoutComponentModule(Engine& engine);
			virtual ~LayoutComponentModule();
			virtual TObject<IReflect>& operator () (IReflect& reflect) override;

			void RequestNew(IScript::Request& request);
			void RequestGetScrollSize(IScript::Request& request, IScript::Delegate<LayoutComponent> layoutComponent);
			void RequestGetScrollOffset(IScript::Request& request, IScript::Delegate<LayoutComponent> layoutComponent);
			void RequestSetScrollOffset(IScript::Request& request, IScript::Delegate<LayoutComponent> layoutComponent, Float2& position);
			void RequestSetLayout(IScript::Request& request, IScript::Delegate<LayoutComponent> layoutComponent, const String& layout);
			void RequestSetWeight(IScript::Request& request, IScript::Delegate<LayoutComponent> layoutComponent, int64_t weight);
			void RequestGetWeight(IScript::Request& request, IScript::Delegate<LayoutComponent> layoutComponent);
			void RequestSetRect(IScript::Request& request, IScript::Delegate<LayoutComponent> layoutComponent, Float4& rect);
			void RequestGetRect(IScript::Request& request, IScript::Delegate<LayoutComponent> layoutComponent);
			void RequestGetClippedRect(IScript::Request& request, IScript::Delegate<LayoutComponent> layoutComponent);
			void RequestSetSize(IScript::Request& request, IScript::Delegate<LayoutComponent> layoutComponent, Float4& size);
			void RequestGetSize(IScript::Request& request, IScript::Delegate<LayoutComponent> layoutComponent);
			void RequestSetPadding(IScript::Request& request, IScript::Delegate<LayoutComponent> layoutComponent, Float4& padding);
			void RequestGetPadding(IScript::Request& request, IScript::Delegate<LayoutComponent> layoutComponent);
			void RequestSetMargin(IScript::Request& request, IScript::Delegate<LayoutComponent> layoutComponent, Float4& margin);
			void RequestGetMargin(IScript::Request& request, IScript::Delegate<LayoutComponent> layoutComponent);
			void RequestSetFitContent(IScript::Request& request, IScript::Delegate<LayoutComponent> window, bool fitContent);
			void RequestGetFitContent(IScript::Request& request, IScript::Delegate<LayoutComponent> window);
			void RequestSetIndexRange(IScript::Request& request, IScript::Delegate<LayoutComponent> layoutComponent, int start, int count);
		};
	}
}


#endif // __LAYOUTCOMPONENTMODULE_H__