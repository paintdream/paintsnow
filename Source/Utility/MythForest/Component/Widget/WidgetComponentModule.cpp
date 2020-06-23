#include "WidgetComponentModule.h"
#include "../../../SnowyStream/SnowyStream.h"

using namespace PaintsNow;
using namespace PaintsNow::NsMythForest;
using namespace PaintsNow::NsSnowyStream;

WidgetComponentModule::WidgetComponentModule(Engine& engine) : BaseClass(engine) {
}

void WidgetComponentModule::Initialize() {
	widgetMesh = engine.snowyStream.CreateReflectedResource(UniqueType<MeshResource>(), "[Runtime]/MeshResource/StandardSquare");
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
	}

	return *this;
}

TShared<WidgetComponent> WidgetComponentModule::RequestNew(IScript::Request& request) {
	CHECK_REFERENCES_NONE();

	TShared<WidgetComponent> widgetComponent = TShared<WidgetComponent>::From(allocator->New(*widgetMesh()));
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
