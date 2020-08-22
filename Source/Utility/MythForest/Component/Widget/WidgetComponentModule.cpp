#include "WidgetComponentModule.h"
#include "../../../SnowyStream/SnowyStream.h"

using namespace PaintsNow;

WidgetComponentModule::WidgetComponentModule(Engine& engine) : BaseClass(engine) {
	batchComponentModule = (engine.GetComponentModuleFromName("BatchComponent")->QueryInterface(UniqueType<BatchComponentModule>()));
	assert(batchComponentModule != nullptr);
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
		ReflectMethod(RequestSetWidgetRepeatMode)[ScriptMethod = "SetWidgetRepeatMode"];
	}

	return *this;
}

TShared<WidgetComponent> WidgetComponentModule::RequestNew(IScript::Request& request, IScript::Delegate<BatchComponent> batchComponent, IScript::Delegate<BatchComponent> batchInstancedDataComponent) {
	CHECK_REFERENCES_NONE();
	CHECK_DELEGATE(batchComponent);
	CHECK_DELEGATE(batchInstancedDataComponent);

	// must batch!
	TShared<BatchComponent> batchUniform = batchComponent.Get();
	TShared<BatchComponent> batchInstanced = batchInstancedDataComponent.Get();
	TShared<WidgetComponent> widgetComponent = TShared<WidgetComponent>::From(allocator->New(widgetMesh, batchUniform, batchInstanced));
	widgetComponent->AddMaterial(0, widgetMaterial);
	widgetComponent->SetWarpIndex(engine.GetKernel().GetCurrentWarpIndex());
	return widgetComponent;
}

void WidgetComponentModule::RequestSetWidgetTexture(IScript::Request& request, IScript::Delegate<WidgetComponent> widgetComponent, IScript::Delegate<TextureResource> texture) {
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
