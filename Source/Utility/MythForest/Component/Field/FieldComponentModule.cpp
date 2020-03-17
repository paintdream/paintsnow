#include "FieldComponentModule.h"
#include "../../Engine.h"

using namespace PaintsNow;
using namespace PaintsNow::NsMythForest;
using namespace PaintsNow::NsSnowyStream;

FieldComponentModule::FieldComponentModule(Engine& engine) : BaseClass(engine) {}
FieldComponentModule::~FieldComponentModule() {}

TObject<IReflect>& FieldComponentModule::operator () (IReflect& reflect) {
	BaseClass::operator () (reflect);

	if (reflect.IsReflectMethod()) {
		ReflectMethod(RequestNew)[ScriptMethod = "New"];
		ReflectMethod(RequestFromSimplygon)[ScriptMethod = "FromSimplygon"];
		ReflectMethod(RequestFromTexture)[ScriptMethod = "FromTexture"];
		ReflectMethod(RequestFromMesh)[ScriptMethod = "FromMesh"];
		ReflectMethod(RequestQuery)[ScriptMethod = "Query"];
	}

	return *this;
}

void FieldComponentModule::RequestNew(IScript::Request& request) {
	CHECK_REFERENCES_NONE();

	TShared<FieldComponent> fieldComponent = TShared<FieldComponent>::From(allocator->New());
	fieldComponent->SetWarpIndex(engine.GetKernel().GetCurrentWarpIndex());
	engine.GetKernel().YieldCurrentWarp();
	request.DoLock();
	request << fieldComponent;
	request.UnLock();
}

void FieldComponentModule::RequestQuery(IScript::Request& request, IScript::Delegate<FieldComponent> fieldComponent, const Float3& position) {
	CHECK_REFERENCES_NONE();
	CHECK_DELEGATE(fieldComponent);

	Bytes result = (*fieldComponent.Get())[position];
	engine.GetKernel().YieldCurrentWarp();

	request.DoLock();
	request << String((char*)result.GetData(), result.GetSize());
	request.UnLock();
}

void FieldComponentModule::RequestFromSimplygon(IScript::Request& request, IScript::Delegate<FieldComponent> fieldComponent, const String& shapeType) {
	CHECK_REFERENCES_NONE();
	CHECK_DELEGATE(fieldComponent);

}

void FieldComponentModule::RequestFromTexture(IScript::Request& request, IScript::Delegate<FieldComponent> fieldComponent, IScript::Delegate<NsSnowyStream::TextureResource> textureResource) {
	CHECK_REFERENCES_NONE();
	CHECK_DELEGATE(fieldComponent);
	CHECK_DELEGATE(textureResource);

}

void FieldComponentModule::RequestFromMesh(IScript::Request& request, IScript::Delegate<FieldComponent> fieldComponent, IScript::Delegate<NsSnowyStream::MeshResource> meshResource) {
	CHECK_REFERENCES_NONE();
	CHECK_DELEGATE(fieldComponent);
	CHECK_DELEGATE(meshResource);
	
}
