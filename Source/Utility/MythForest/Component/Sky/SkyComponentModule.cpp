#include "SkyComponentModule.h"

using namespace PaintsNow;

SkyComponentModule::SkyComponentModule(Engine& engine) : BaseClass(engine) {
	batchComponentModule = (engine.GetComponentModuleFromName("BatchComponent")->QueryInterface(UniqueType<BatchComponentModule>()));
	assert(batchComponentModule != nullptr);
}

TObject<IReflect>& SkyComponentModule::operator () (IReflect& reflect) {
	BaseClass::operator () (reflect);

	if (reflect.IsReflectMethod()) {
		ReflectMethod(RequestNew)[ScriptMethodLocked = "New"];
	}

	return *this;
}

TShared<SkyComponent> SkyComponentModule::RequestNew(IScript::Request& request, IScript::Delegate<MeshResource> meshResource,  IScript::Delegate<BatchComponent> batch) {
	CHECK_REFERENCES_NONE();
	CHECK_DELEGATE(meshResource);

	TShared<BatchComponent> batchComponent;
	if (batch.Get() == nullptr) {
		batchComponent = batchComponentModule->Create(IRender::Resource::BufferDescription::UNIFORM);
	} else {
		batchComponent = batch.Get();
	}

	assert(batchComponent->GetWarpIndex() == engine.GetKernel().GetCurrentWarpIndex());
	TShared<MeshResource> res = meshResource.Get();
	TShared<SkyComponent> modelComponent = TShared<SkyComponent>::From(allocator->New(res, batchComponent));
	modelComponent->SetWarpIndex(engine.GetKernel().GetCurrentWarpIndex());

	return modelComponent;
}
