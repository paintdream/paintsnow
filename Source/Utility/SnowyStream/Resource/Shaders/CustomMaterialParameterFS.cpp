#include "CustomMaterialParameterFS.h"

using namespace PaintsNow;

CustomMaterialParameterFS::CustomMaterialParameterFS() {
	description = TShared<CustomShaderDescription>::From(new CustomShaderDescription());
}

String CustomMaterialParameterFS::GetShaderText() {
	return description->code;
}

TObject<IReflect>& CustomMaterialParameterFS::operator () (IReflect& reflect) {
	BaseClass::operator () (reflect);

	if (reflect.IsReflectProperty()) {
	}

	return *this;
}
