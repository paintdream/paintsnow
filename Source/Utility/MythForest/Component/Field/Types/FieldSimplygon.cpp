#include "FieldSimplygon.h"

using namespace PaintsNow;

FieldSimplygon::FieldSimplygon(SIMPOLYGON_TYPE t) : type(t) {

}

TObject<IReflect>& FieldSimplygon::operator () (IReflect& reflect) {
	BaseClass::operator () (reflect);
	if (reflect.IsReflectProperty()) {
		ReflectProperty(type);
	}

	return *this;
}

Bytes FieldSimplygon::operator [] (const Float3& position) const {
	bool result = false;
	switch (type) {
	case BOUNDING_BOX:
		result = position.x() >= -1.0f && position.x() <= 1.0f
			&& position.y() >= -1.0f && position.y() <= 1.0f
			&& position.z() >= -1.0f && position.z() <= 1.0f;
		break;
	case BOUNDING_SPHERE:
		result = position.SquareLength() <= 1.0f;
		break;
	case BOUNDING_CYLINDER:
		result = position.x() * position.x() + position.y() * position.y() <= 1.0f
			&& position.z() >= -1.0f && position.z() <= 1.0f;
		break;
	}
	
	Bytes encoder;
	encoder.Assign((const uint8_t*)&result, sizeof(bool));
	return result;
}