#include "Module.h"
#include "Engine.h"
#include "../../General/Interface/Interfaces.h"

using namespace PaintsNow;
using namespace PaintsNow::NsMythForest;

Module::Module(Engine& e) : engine(e) {}

void Module::TickFrame() {}

TObject<IReflect>& Module::operator () (IReflect& reflect) {
	BaseClass::operator () (reflect);

	if (reflect.IsReflectProperty()) {
	}

	return *this;
}