#include "ParticleComponentModule.h"
#include "../../Engine.h"

using namespace PaintsNow;
using namespace PaintsNow::NsMythForest;
using namespace PaintsNow::NsSnowyStream;

ParticleComponentModule::ParticleComponentModule(Engine& engine) : BaseClass(engine) {}
ParticleComponentModule::~ParticleComponentModule() {}

TObject<IReflect>& ParticleComponentModule::operator () (IReflect& reflect) {
	BaseClass::operator () (reflect);
	if (reflect.IsReflectMethod()) {
		ReflectMethod(RequestNew)[ScriptMethod = "New"];
		ReflectMethod(RequestRebuild)[ScriptMethod = "Rebuild"];
	}

	return *this;
}

TShared<ParticleComponent> ParticleComponentModule::RequestNew(IScript::Request& request) {
	CHECK_REFERENCES_NONE();

	TShared<ParticleComponent> particleComponent = TShared<ParticleComponent>::From(allocator->New());
	particleComponent->SetWarpIndex(engine.GetKernel().GetCurrentWarpIndex());
	return particleComponent;
}

void ParticleComponentModule::RequestRebuild(IScript::Request& request, IScript::Delegate<ParticleComponent> particleComponent) {
	CHECK_REFERENCES_NONE();
	CHECK_DELEGATE(particleComponent);

}

