#include "ShapeComponentModule.h"
#include "../../Engine.h"

using namespace PaintsNow;

ShapeComponentModule::ShapeComponentModule(Engine& engine) : BaseClass(engine) {}
ShapeComponentModule::~ShapeComponentModule() {}

TObject<IReflect>& ShapeComponentModule::operator () (IReflect& reflect) {
	BaseClass::operator () (reflect);

	if (reflect.IsReflectMethod()) {
		ReflectMethod(RequestNew)[ScriptMethod = "New"];
		ReflectMethod(RequestRebuild)[ScriptMethod = "Rebuild"];
	}

	return *this;
}

TShared<ShapeComponent> ShapeComponentModule::RequestNew(IScript::Request& request, IScript::Delegate<MeshResource> mesh) {
	CHECK_REFERENCES_NONE();
	CHECK_DELEGATE(mesh);

	engine.GetKernel().YieldCurrentWarp();

	TShared<ShapeComponent> shapeComponent = TShared<ShapeComponent>::From(allocator->New());
	shapeComponent->SetWarpIndex(engine.GetKernel().GetCurrentWarpIndex());
	shapeComponent->Update(engine, mesh.Get());
	return shapeComponent;
}

void ShapeComponentModule::RequestRebuild(IScript::Request& request, IScript::Delegate<ShapeComponent> shapeComponent, Float4& color) {
	CHECK_REFERENCES_NONE();
	CHECK_DELEGATE(shapeComponent);

}