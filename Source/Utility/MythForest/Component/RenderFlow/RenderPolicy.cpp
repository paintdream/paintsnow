#include "RenderPolicy.h"

using namespace PaintsNow;

RenderPolicy::RenderPolicy() : priority(0) {}

TObject<IReflect>& RenderPolicy::operator () (IReflect& reflect) {
	BaseClass::operator () (reflect);

	if (reflect.IsReflectProperty()) {
		ReflectProperty(renderPortName);
		ReflectProperty(priority);
	}

	return *this;
}