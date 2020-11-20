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

template <int type, class Q>
struct BoxQueryer {
	BoxQueryer(Q& q) : queryer(q) {}
	Q& queryer;

	bool operator () (const Float3Pair& b, TKdTree<Float3Pair, Unit>& tree) const {
		if (type == FieldSimplygon::BOUNDING_BOX) {
			queryer(static_cast<Entity&>(tree));
		} else if (type == FieldSimplygon::BOUNDING_SPHERE) {
			Float3 from = ToLocal(b, tree.GetKey().first);
			Float3 to = ToLocal(b, tree.GetKey().second);
			
			float dx = Math::Max(0.0f, Math::Max(from.x(), -to.x()));
			float dy = Math::Max(0.0f, Math::Max(from.y(), -to.y()));
			float dz = Math::Max(0.0f, Math::Max(from.z(), -to.z()));

			if (dx * dx + dy * dy + dz * dz <= 1.0f) {
				queryer(static_cast<Entity&>(tree));
			}
		} else if (type == FieldSimplygon::BOUNDING_CYLINDER) {
			Float3 from = ToLocal(b, tree.GetKey().first);
			Float3 to = ToLocal(b, tree.GetKey().second);
			
			float dx = Math::Max(0.0f, Math::Max(from.x(), -to.x()));
			float dy = Math::Max(0.0f, Math::Max(from.y(), -to.y()));

			if (dx * dx + dy * dy <= 1.0f) {
				queryer(static_cast<Entity&>(tree));
			}
		}

		return true; // search all
	}
};

struct Poster {
	Poster(Event& e, Tiny::FLAG m) : event(e), mask(m) {}

	Event& event;
	Tiny::FLAG mask;

	void operator () (Entity& entity) {
		entity.PostEvent(event, mask);
	}
};

void FieldSimplygon::PostEventForEntityTree(Entity* entity, Event& event, FLAG mask) const {
	Poster poster(event, mask);
	switch (type) {
	case BOUNDING_BOX:
	{
		BoxQueryer<BOUNDING_BOX, Poster> q(poster);
		entity->Query(box, q);
		break;
	}
	case BOUNDING_SPHERE:
	{
		BoxQueryer<BOUNDING_SPHERE, Poster> q(poster);
		entity->Query(box, q);
		break;
	}
	case BOUNDING_CYLINDER:
	{
		BoxQueryer<BOUNDING_CYLINDER, Poster> q(poster);
		entity->Query(box, q);
		break;
	}
	}
}

struct Collector {
	Collector(std::vector<TShared<Entity> >& e) : entities(e) {}
	std::vector<TShared<Entity> >& entities;

	void operator () (Entity& entity) {
		entities.push_back(&entity);
	}
};


void FieldSimplygon::QueryEntitiesForEntityTree(Entity* entity, std::vector<TShared<Entity> >& entities) const {
	Collector collector(entities);
	switch (type) {
	case BOUNDING_BOX:
	{
		BoxQueryer<BOUNDING_BOX, Collector> q(collector);
		entity->Query(box, q);
		break;
	}
	case BOUNDING_SPHERE:
	{
		BoxQueryer<BOUNDING_SPHERE, Collector> q(collector);
		entity->Query(box, q);
		break;
	}
	case BOUNDING_CYLINDER:
	{
		BoxQueryer<BOUNDING_CYLINDER, Collector> q(collector);
		entity->Query(box, q);
		break;
	}
	}
}
