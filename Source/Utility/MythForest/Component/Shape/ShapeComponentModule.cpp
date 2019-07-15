#include "ShapeComponentModule.h"
#include "../../Engine.h"

using namespace PaintsNow;
using namespace PaintsNow::NsMythForest;
using namespace PaintsNow::NsSnowyStream;

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

void ShapeComponentModule::RequestNew(IScript::Request& request, IScript::Delegate<MeshResource> mesh) {
	CHECK_REFERENCES_NONE();
	CHECK_DELEGATE(mesh);

	TShared<ShapeComponent> shapeComponent = TShared<ShapeComponent>::From(allocator->New());
	shapeComponent->SetWarpIndex(engine.GetKernel().GetCurrentWarpIndex());
	shapeComponent->Update(engine, mesh.Get());
	engine.GetKernel().YieldCurrentWarp();

	request.DoLock();
	request << shapeComponent;
	request.UnLock();
}

void ShapeComponentModule::RequestRebuild(IScript::Request& request, IScript::Delegate<ShapeComponent> shapeComponent, Float4& color) {
	CHECK_REFERENCES_NONE();
	CHECK_DELEGATE(shapeComponent);

}