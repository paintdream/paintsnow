#include "FieldSimplygon.h"

using namespace PaintsNow;

FieldSimplygon::FieldSimplygon(SIMPOLYGON_TYPE t, const Float3Pair& b) : type(t), box(b) {

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
		result = Contain(box, position);
		break;
	case BOUNDING_SPHERE:
		{
			Float3 local = ToLocal(box, position);
			result = local.SquareLength() <= 1.0f;
		}
		break;
	case BOUNDING_CYLINDER:
		{
			Float3 local = ToLocal(box, position);
			if (local.z() < -1.0f || local.z() > 1.0f) {
				result = false;
			} else {
				result = Float2(local.x(), local.y()).SquareLength() <= 1.0f;
			}
		}
		break;
	}
	
	Bytes encoder;
	encoder.Assign((const uint8_t*)&result, sizeof(bool));
	return result;
}

struct BoxQueryer {
	BoxQueryer(Event& e, Tiny::FLAG m) : event(e), mask(m) {}

	Event& event;
	Tiny::FLAG mask;

	bool operator () (const Float3Pair& b, TKdTree<Float3Pair, Unit>& tree) const {
		static_cast<Entity&>(tree).PostEvent(event, mask);
		return true; // always matched
	}
};

void FieldSimplygon::PostEventForEntityTree(Entity* entity, Event& event, FLAG mask) const {
	// by now we only support box
	// TODO: add sphere and cylinder
	BoxQueryer q(event, mask);
	entity->Query(box, q);
}
