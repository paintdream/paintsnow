// TextViewComponentModule.h
// By PaintDream (paintdream@paintdream.com)
// 2015-5-5
//

#pragma once
#include "TextViewComponent.h"
#include "../Batch/BatchComponentModule.h"
#include "../../Module.h"

namespace PaintsNow {
	class Entity;
	class TextViewComponent;
	class TextViewComponentModule : public TReflected<TextViewComponentModule, ModuleImpl<TextViewComponent> > {
	public:
		TextViewComponentModule(Engine& engine);
		virtual ~TextViewComponentModule();
		virtual TObject<IReflect>& operator () (IReflect& reflect) override;

		TShared<TextViewComponent> RequestNew(IScript::Request& request, IScript::Delegate<FontResource> fontResource, IScript::Delegate<BatchComponent> batch, IScript::Delegate<MeshResource> meshResource, IScript::Delegate<MaterialResource> materialResource);
		void RequestSetFontSize(IScript::Request& request, IScript::Delegate<TextViewComponent> textViewComponent, uint32_t fontSize);
		String RequestGetText(IScript::Request& request, IScript::Delegate<TextViewComponent> textViewComponent);
		void RequestSetText(IScript::Request& request, IScript::Delegate<TextViewComponent> window, const String& text);
		Short3 RequestLocateText(IScript::Request& request, IScript::Delegate<TextViewComponent> window, Short2& offset, bool isRowCol);

	protected:
		TShared<MaterialResource> defaultTextMaterial;
		TShared<MeshResource> defaultTextMesh;
		BatchComponentModule* batchComponentModule;
	};
}

