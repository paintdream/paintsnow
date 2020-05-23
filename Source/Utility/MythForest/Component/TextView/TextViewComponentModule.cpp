#include "TextViewComponentModule.h"
#include "../../Engine.h"

using namespace PaintsNow;
using namespace PaintsNow::NsMythForest;
using namespace PaintsNow::NsSnowyStream;

TextViewComponentModule::TextViewComponentModule(Engine& engine) : BaseClass(engine) {}
TextViewComponentModule::~TextViewComponentModule() {}

TObject<IReflect>& TextViewComponentModule::operator () (IReflect& reflect) {
	BaseClass::operator () (reflect);
	if (reflect.IsReflectMethod()) {
		ReflectMethod(RequestNew)[ScriptMethod = "New"];
		ReflectMethod(RequestSetFont)[ScriptMethod = "SetFont"];
		ReflectMethod(RequestGetText)[ScriptMethod = "GetText"];
		ReflectMethod(RequestSetText)[ScriptMethod = "SetText"];
		ReflectMethod(RequestLocateText)[ScriptMethod = "LocateText"];
	}

	return *this;
}

void TextViewComponentModule::RequestNew(IScript::Request& request) {
	CHECK_REFERENCES_NONE();

	TShared<TextViewComponent> textViewComponent = TShared<TextViewComponent>::From(allocator->New());
	textViewComponent->SetWarpIndex(engine.GetKernel().GetCurrentWarpIndex());

	engine.GetKernel().YieldCurrentWarp();
	request.DoLock();
	request << textViewComponent;
	request.UnLock();
}

void TextViewComponentModule::RequestSetFont(IScript::Request& request, IScript::Delegate<TextViewComponent> textViewComponent, const String& font, int64_t fontSize, float reinforce) {
	CHECK_REFERENCES_NONE();
	CHECK_DELEGATE(textViewComponent);

	// Get font
	TShared<FontResource> fontResource = engine.snowyStream.CreateReflectedResource(UniqueType<FontResource>(), font);
	if (fontResource) {
		textViewComponent->mainFont = fontResource;
		textViewComponent->fontSize = safe_cast<uint32_t>(fontSize);
		textViewComponent->SetUpdateMark();
	}
}

void TextViewComponentModule::RequestGetText(IScript::Request& request, IScript::Delegate<TextViewComponent> textViewComponent) {
	CHECK_REFERENCES_NONE();
	CHECK_DELEGATE(textViewComponent);
	engine.GetKernel().YieldCurrentWarp();

	request.DoLock();
	request << textViewComponent->text;
	request.UnLock();
}

void TextViewComponentModule::RequestSetText(IScript::Request& request, IScript::Delegate<TextViewComponent> textViewComponent, const String& text) {
	CHECK_REFERENCES_NONE();
	CHECK_DELEGATE(textViewComponent);

	textViewComponent->SetText(text);
	textViewComponent->Flag() |= Tiny::TINY_MODIFIED;
	textViewComponent->SetUpdateMark();
}

void TextViewComponentModule::RequestLocateText(IScript::Request& request, IScript::Delegate<TextViewComponent> textViewComponent, Int2& offset, bool isRowCol) {
	CHECK_REFERENCES_NONE();
	CHECK_DELEGATE(textViewComponent);

	Int2 rowCol;

	// TODO: fix offset
	int loc = textViewComponent->Locate(rowCol, offset/* - textViewComponent->clippedRect.first*/, isRowCol);
	engine.GetKernel().YieldCurrentWarp();
	request.DoLock();
	request << Int3(rowCol.x(), rowCol.y(), loc);
	request.UnLock();
}