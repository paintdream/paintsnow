#include "WidgetComponentModule.h"
#include "../../../SnowyStream/SnowyStream.h"

using namespace PaintsNow;
using namespace PaintsNow::NsMythForest;
using namespace PaintsNow::NsSnowyStream;

WidgetComponentModule::WidgetComponentModule(Engine& engine) : BaseClass(engine) {
}

void WidgetComponentModule::Initialize() {
	SnowyStream& snowyStream = engine.snowyStream;
	widgetMesh = snowyStream.CreateReflectedResource(UniqueType<MeshResource>(), "[Runtime]/MeshResource/StandardSquare");
	widgetMaterial = snowyStream.CreateReflectedResource(UniqueType<MaterialResource>(), "[Runtime]/MaterialResource/Widget");
}

void WidgetComponentModule::Uninitialize() {
	widgetMesh = nullptr;
}

TObject<IReflect>& WidgetComponentModule::operator () (IReflect& reflect) {
	BaseClass::operator () (reflect);

	if (reflect.IsReflectMethod()) {
		ReflectMethod(RequestNew)[ScriptMethod = "New"];
		ReflectMethod(RequestSetWidgetTexture)[ScriptMethod = "SetWidgetTexture"];
		ReflectMethod(RequestSetWidgetCoord)[ScriptMethod = "SetWidgetCoord"];
		ReflectMethod(RequestSetWidgetMaterial)[ScriptMethod = "SetWidgetMaterial"];
		ReflectMethod(RequestSetWidgetRepeatMode)[ScriptMethod = "SetWidgetRepeatMode"];
	}

	return *this;
}

TShared<WidgetComponent> WidgetComponentModule::RequestNew(IScript::Request& request) {
	CHECK_REFERENCES_NONE();

	TShared<WidgetComponent> widgetComponent = TShared<WidgetComponent>::From(allocator->New(std::ref(*widgetMesh()), widgetMaterial()));
	widgetComponent->SetWarpIndex(engine.GetKernel().GetCurrentWarpIndex());
	return widgetComponent;
}


void WidgetComponentModule::RequestSetWidgetTexture(IScript::Request& request, IScript::Delegate<WidgetComponent> widgetComponent, IScript::Delegate<NsSnowyStream::TextureResource> texture) {
	CHECK_REFERENCES_NONE();
	CHECK_DELEGATE(widgetComponent);
	CHECK_THREAD_IN_MODULE(widgetComponent);

	widgetComponent->mainTexture = texture.Get();
}

void WidgetComponentModule::RequestSetWidgetCoord(IScript::Request& request, IScript::Delegate<WidgetComponent> widgetComponent, Float4& inCoord, Float4& outCoord) {
	CHECK_REFERENCES_NONE();
	CHECK_DELEGATE(widgetComponent);
	CHECK_THREAD_IN_MODULE(widgetComponent);

	widgetComponent->inTexCoordRect = inCoord;
	widgetComponent->outTexCoordRect = outCoord;
}

void WidgetComponentModule::RequestSetWidgetMaterial(IScript::Request& request, IScript::Delegate<WidgetComponent> widgetComponent, IScript::Delegate<NsSnowyStream::MaterialResource> material) {
	CHECK_REFERENCES_NONE();
	CHECK_DELEGATE(widgetComponent);
	CHECK_THREAD_IN_MODULE(widgetComponent);

	widgetComponent->materialResource = material.Get();
}

void WidgetComponentModule::RequestSetWidgetRepeatMode(IScript::Request& request, IScript::Delegate<WidgetComponent> widgetComponent, bool repeatable) {
	CHECK_REFERENCES_NONE();
	CHECK_DELEGATE(widgetComponent);
	CHECK_THREAD_IN_MODULE(widgetComponent);

	if (repeatable) {
		widgetComponent->Flag().fetch_or(repeatable ? WidgetComponent::WIDGETCOMPONENT_TEXTURE_REPEATABLE : 0);
	} else {
		widgetComponent->Flag().fetch_and(~(repeatable ? WidgetComponent::WIDGETCOMPONENT_TEXTURE_REPEATABLE : 0));
	}
}
