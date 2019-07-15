#include "RenderPolicy.h"

using namespace PaintsNow;
using namespace PaintsNow::NsMythForest;

RenderPolicy::RenderPolicy() : priority(0) {}

TObject<IReflect>& RenderPolicy::operator () (IReflect& reflect) {
	BaseClass::operator () (reflect);

	if (reflect.IsReflectProperty()) {
		ReflectProperty(renderPortName);
		ReflectProperty(priority);
	}

	return *this;
}