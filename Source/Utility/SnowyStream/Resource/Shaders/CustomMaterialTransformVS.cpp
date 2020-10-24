#include "CustomMaterialTransformVS.h"

using namespace PaintsNow;

CustomMaterialTransformVS::CustomMaterialTransformVS() {
	description = TShared<CustomShaderDescription>::From(new CustomShaderDescription());
}

String CustomMaterialTransformVS::GetShaderText() {
	return description->code;
}

TObject<IReflect>& CustomMaterialTransformVS::operator () (IReflect& reflect) {
	BaseClass::operator () (reflect);

	if (reflect.IsReflectProperty()) {
		description->ReflectExternal(reflect, uniformData, optionData);
	}

	return *this;
}
