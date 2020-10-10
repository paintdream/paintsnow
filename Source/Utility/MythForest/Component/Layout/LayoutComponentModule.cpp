#include "LayoutComponentModule.h"
#include "../../Engine.h"

using namespace PaintsNow;

LayoutComponentModule::LayoutComponentModule(Engine& engine) : BaseClass(engine) {}
LayoutComponentModule::~LayoutComponentModule() {}

TObject<IReflect>& LayoutComponentModule::operator () (IReflect& reflect) {
	BaseClass::operator () (reflect);
	if (reflect.IsReflectMethod()) {
		ReflectMethod(RequestNew)[ScriptMethod = "New"];
		ReflectMethod(RequestGetScrollSize)[ScriptMethod = "GetScrollSize"];
		ReflectMethod(RequestGetScrollOffset)[ScriptMethod = "GetScrollOffset"];
		ReflectMethod(RequestSetScrollOffset)[ScriptMethod = "SetScrollOffset"];
		ReflectMethod(RequestSetLayout)[ScriptMethod = "SetLayout"];
		ReflectMethod(RequestSetWeight)[ScriptMethod = "SetWeight"];
		ReflectMethod(RequestGetWeight)[ScriptMethod = "GetWeight"];
		ReflectMethod(RequestSetRect)[ScriptMethod = "SetRect"];
		ReflectMethod(RequestGetRect)[ScriptMethod = "GetRect"];
		ReflectMethod(RequestGetClippedRect)[ScriptMethod = "GetClippedRect"];
		ReflectMethod(RequestSetSize)[ScriptMethod = "SetSize"];
		ReflectMethod(RequestGetSize)[ScriptMethod = "GetSize"];
		ReflectMethod(RequestSetPadding)[ScriptMethod = "SetPadding"];
		ReflectMethod(RequestGetPadding)[ScriptMethod = "GetPadding"];
		ReflectMethod(RequestSetMargin)[ScriptMethod = "SetMargin"];
		ReflectMethod(RequestGetMargin)[ScriptMethod = "GetMargin"];
		ReflectMethod(RequestSetFitContent)[ScriptMethod = "SetFitContent"];
		ReflectMethod(RequestGetFitContent)[ScriptMethod = "GetFitContent"];
		ReflectMethod(RequestSetIndexRange)[ScriptMethod = "SetIndexRange"];
	}

	return *this;
}

TShared<LayoutComponent> LayoutComponentModule::RequestNew(IScript::Request& request) {
	CHECK_REFERENCES_NONE();

	TShared<LayoutComponent> layoutComponent = TShared<LayoutComponent>::From(allocator->New());
	layoutComponent->SetWarpIndex(engine.GetKernel().GetCurrentWarpIndex());

	return layoutComponent;
}

Float4 LayoutComponentModule::RequestGetClippedRect(IScript::Request& request, IScript::Delegate<LayoutComponent> layoutComponent) {
	CHECK_REFERENCES_NONE();
	CHECK_DELEGATE(layoutComponent);

	return (Float4)layoutComponent->clippedRect;
}

Float4 LayoutComponentModule::RequestGetRect(IScript::Request& request, IScript::Delegate<LayoutComponent> layoutComponent) {
	CHECK_REFERENCES_NONE();
	CHECK_DELEGATE(layoutComponent);

	return Float4(layoutComponent->rect);
}

void LayoutComponentModule::RequestSetLayout(IScript::Request& request, IScript::Delegate<LayoutComponent> layoutComponent, const String& layout) {
	CHECK_REFERENCES_NONE();
	CHECK_DELEGATE(layoutComponent);

	if (layout == "horizontal") {
		layoutComponent->Flag().fetch_and(~LayoutComponent::LAYOUT_VERTICAL, std::memory_order_release);
	} else {
		layoutComponent->Flag().fetch_or(LayoutComponent::LAYOUT_VERTICAL, std::memory_order_acquire);
	}
}

void LayoutComponentModule::RequestSetFitContent(IScript::Request& request, IScript::Delegate<LayoutComponent> layoutComponent, bool fitContent) {
	CHECK_REFERENCES_NONE();
	CHECK_DELEGATE(layoutComponent);

	layoutComponent->Flag().fetch_or(LayoutComponent::LAYOUT_ADAPTABLE, std::memory_order_acquire);
	layoutComponent->SetUpdateMark();
}

bool LayoutComponentModule::RequestGetFitContent(IScript::Request& request, IScript::Delegate<LayoutComponent> layoutComponent) {
	CHECK_REFERENCES_NONE();
	CHECK_DELEGATE(layoutComponent);
	return !!(layoutComponent->Flag() & LayoutComponent::LAYOUT_ADAPTABLE);
}

Float2 LayoutComponentModule::RequestGetScrollSize(IScript::Request& request, IScript::Delegate<LayoutComponent> layoutComponent) {
	CHECK_REFERENCES_NONE();
	CHECK_DELEGATE(layoutComponent);

	return layoutComponent->scrollSize;
}

Float2 LayoutComponentModule::RequestGetScrollOffset(IScript::Request& request, IScript::Delegate<LayoutComponent> layoutComponent) {
	CHECK_REFERENCES_NONE();
	CHECK_DELEGATE(layoutComponent);

	return layoutComponent->scrollOffset;
}

void LayoutComponentModule::RequestSetScrollOffset(IScript::Request& request, IScript::Delegate<LayoutComponent> layoutComponent, const Float2& position) {
	CHECK_REFERENCES_NONE();
	CHECK_DELEGATE(layoutComponent);

	layoutComponent->scrollOffset = position;
	layoutComponent->SetUpdateMark();
}

void LayoutComponentModule::RequestSetRect(IScript::Request& request, IScript::Delegate<LayoutComponent> layoutComponent, const Float4& rect) {
	CHECK_REFERENCES_NONE();
	CHECK_DELEGATE(layoutComponent);

	// assert(rect.x() >= 0 && rect.y() >= 0 && rect.z() >= 0 && rect.w() >= 0);
	
	layoutComponent->rect = Float2Pair(Float2(rect.x(), rect.y()), Float2(rect.z(), rect.w()));
	layoutComponent->SetUpdateMark();
}

void LayoutComponentModule::RequestSetWeight(IScript::Request& request, IScript::Delegate<LayoutComponent> layoutComponent, int64_t weight) {
	CHECK_REFERENCES_NONE();
	CHECK_DELEGATE(layoutComponent);
	
	layoutComponent->weight = (int32_t)weight;
	layoutComponent->SetUpdateMark();
}

int32_t LayoutComponentModule::RequestGetWeight(IScript::Request& request, IScript::Delegate<LayoutComponent> layoutComponent) {
	CHECK_REFERENCES_NONE();
	CHECK_DELEGATE(layoutComponent);
	return layoutComponent->weight;
}

Float4 LayoutComponentModule::RequestGetSize(IScript::Request& request, IScript::Delegate<LayoutComponent> layoutComponent) {
	CHECK_REFERENCES_NONE();
	CHECK_DELEGATE(layoutComponent);

	return Float4(layoutComponent->size);
}

void LayoutComponentModule::RequestSetSize(IScript::Request& request, IScript::Delegate<LayoutComponent> layoutComponent, const Float4& size) {
	CHECK_REFERENCES_NONE();
	CHECK_DELEGATE(layoutComponent);

	layoutComponent->size = Float2Pair(Float2(size.x(), size.y()), Float2(size.z(), size.w()));
	layoutComponent->SetUpdateMark();
}

void LayoutComponentModule::RequestSetPadding(IScript::Request& request, IScript::Delegate<LayoutComponent> layoutComponent, const Float4& size) {
	CHECK_REFERENCES_NONE();
	CHECK_DELEGATE(layoutComponent);
	assert(size.x() >= 0 && size.y() >= 0 && size.z() >= 0 && size.w() >= 0);

	layoutComponent->padding = Float2Pair(Float2(size.x(), size.y()), Float2(size.z(), size.w()));
	layoutComponent->SetUpdateMark();
}

Float4 LayoutComponentModule::RequestGetPadding(IScript::Request& request, IScript::Delegate<LayoutComponent> layoutComponent) {
	CHECK_REFERENCES_NONE();
	CHECK_DELEGATE(layoutComponent);

	return Float4(layoutComponent->padding);
}

void LayoutComponentModule::RequestSetMargin(IScript::Request& request, IScript::Delegate<LayoutComponent> layoutComponent, const Float4& size) {
	CHECK_REFERENCES_NONE();
	CHECK_DELEGATE(layoutComponent);
	
	layoutComponent->margin = Float2Pair(Float2(size.x(), size.y()), Float2(size.z(), size.w()));
	layoutComponent->SetUpdateMark();
}

Float4 LayoutComponentModule::RequestGetMargin(IScript::Request& request, IScript::Delegate<LayoutComponent> layoutComponent) {
	CHECK_REFERENCES_NONE();
	CHECK_DELEGATE(layoutComponent);

	return Float4(layoutComponent->margin);
}

void LayoutComponentModule::RequestSetIndexRange(IScript::Request& request, IScript::Delegate<LayoutComponent> layoutComponent, int start, int count) {
	CHECK_REFERENCES_NONE();
	CHECK_DELEGATE(layoutComponent);

	layoutComponent->start = start;
	layoutComponent->count = count;
	layoutComponent->SetUpdateMark();
}

