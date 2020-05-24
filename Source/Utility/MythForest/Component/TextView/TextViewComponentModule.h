// TextViewComponentModule.h
// By PaintDream (paintdream@paintdream.com)
// 2015-5-5
//

#ifndef __TEXTVIEWCOMPONENTMODULE_H__
#define __TEXTVIEWCOMPONENTMODULE_H__

#include "TextViewComponent.h"
#include "../../Module.h"

namespace PaintsNow {
	namespace NsMythForest {
		class Entity;
		class TextViewComponent;
		class TextViewComponentModule  : public TReflected<TextViewComponentModule, ModuleImpl<TextViewComponent> > {
		public:
			TextViewComponentModule(Engine& engine);
			virtual ~TextViewComponentModule();
			virtual TObject<IReflect>& operator () (IReflect& reflect) override;

			void RequestNew(IScript::Request& request);
			void RequestSetFont(IScript::Request& request, IScript::Delegate<TextViewComponent> textViewComponent, const String& font, int64_t fontSize, float reinforce);
			void RequestGetText(IScript::Request& request, IScript::Delegate<TextViewComponent> textViewComponent);
			void RequestSetText(IScript::Request& request, IScript::Delegate<TextViewComponent> window, const String& text);
			void RequestLocateText(IScript::Request& request, IScript::Delegate<TextViewComponent> window, Int2& offset, bool isRowCol);

		protected:
			TShared<NsSnowyStream::MaterialResource> defaultTextMaterial;
		};
	}
}


#endif // __TEXTVIEWCOMPONENTMODULE_H__