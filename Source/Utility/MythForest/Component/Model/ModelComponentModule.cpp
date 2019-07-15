#include "ModelComponentModule.h"

using namespace PaintsNow;
using namespace PaintsNow::NsMythForest;
using namespace PaintsNow::NsSnowyStream;

ModelComponentModule::ModelComponentModule(Engine& engine, BatchComponentModule& batchModule) : BaseClass(engine), batchComponentModule(batchModule) {}

TObject<IReflect>& ModelComponentModule::operator () (IReflect& reflect) {
	BaseClass::operator () (reflect);

	if (reflect.IsReflectMethod()) {
		ReflectMethod(RequestNew)[ScriptMethod = "New"];
		ReflectMethod(RequestAddMaterial)[ScriptMethod = "AddMaterial"];
	}

	return *this;
}

void ModelComponentModule::RequestNew(IScript::Request& request, IScript::Delegate<MeshResource> meshResource,  IScript::Delegate<BatchComponent> batch) {
	CHECK_REFERENCES_NONE();
	CHECK_DELEGATE(meshResource);

	TShared<BatchComponent> batchComponent;
	if (batch.Get() == nullptr) {
		batchComponent = batchComponentModule.Create();
	} else {
		batchComponent = batch.Get();
	}

	TShared<MeshResource> res = meshResource.Get();
	TShared<ModelComponent> modelComponent = TShared<ModelComponent>::From(allocator->New(res, batchComponent));
	modelComponent->SetWarpIndex(engine.GetKernel().GetCurrentWarpIndex());

	engine.GetKernel().YieldCurrentWarp();
	request.DoLock();
	request << modelComponent;
	request.UnLock();
}

void ModelComponentModule::RequestAddMaterial(IScript::Request& request, IScript::Delegate<ModelComponent> modelComponent, uint32_t meshGroupIndex, IScript::Delegate<NsSnowyStream::MaterialResource> materialResource) {
	CHECK_REFERENCES_NONE();
	CHECK_DELEGATE(modelComponent);
	CHECK_DELEGATE(materialResource);

	TShared<MaterialResource> mat = materialResource.Get();
	modelComponent->AddMaterial(meshGroupIndex, mat);
}
