#include "FieldComponentModule.h"
#include "../../Engine.h"
#include "Types/FieldSimplygon.h"

using namespace PaintsNow;

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

TShared<FieldComponent> FieldComponentModule::RequestNew(IScript::Request& request) {
	CHECK_REFERENCES_NONE();

	TShared<FieldComponent> fieldComponent = TShared<FieldComponent>::From(allocator->New());
	fieldComponent->SetWarpIndex(engine.GetKernel().GetCurrentWarpIndex());
	return fieldComponent;
}

String FieldComponentModule::RequestQuery(IScript::Request& request, IScript::Delegate<FieldComponent> fieldComponent, const Float3& position) {
	CHECK_REFERENCES_NONE();
	CHECK_DELEGATE(fieldComponent);

	Bytes result = (*fieldComponent.Get())[position];
	return String((char*)result.GetData(), result.GetSize());
}

void FieldComponentModule::RequestFromSimplygon(IScript::Request& request, IScript::Delegate<FieldComponent> fieldComponent, const String& shapeType) {
	CHECK_REFERENCES_NONE();
	CHECK_DELEGATE(fieldComponent);

	FieldSimplygon::SIMPOLYGON_TYPE type = FieldSimplygon::BOUNDING_BOX;

	if (shapeType == "Box") {
		type = FieldSimplygon::BOUNDING_BOX;
	} else if (shapeType == "Sphere") {
		type = FieldSimplygon::BOUNDING_SPHERE;
	} else if (shapeType == "Cylinder") {
		type = FieldSimplygon::BOUNDING_CYLINDER;
	}

	TShared<FieldSimplygon> instance = TShared<FieldSimplygon>(new FieldSimplygon(type));

	engine.GetKernel().YieldCurrentWarp();
	request.DoLock();
	request << instance();
	request.UnLock();
}

void FieldComponentModule::RequestFromTexture(IScript::Request& request, IScript::Delegate<FieldComponent> fieldComponent, IScript::Delegate<TextureResource> textureResource) {
	CHECK_REFERENCES_NONE();
	CHECK_DELEGATE(fieldComponent);
	CHECK_DELEGATE(textureResource);

}

void FieldComponentModule::RequestFromMesh(IScript::Request& request, IScript::Delegate<FieldComponent> fieldComponent, IScript::Delegate<MeshResource> meshResource) {
	CHECK_REFERENCES_NONE();
	CHECK_DELEGATE(fieldComponent);
	CHECK_DELEGATE(meshResource);
	
}
