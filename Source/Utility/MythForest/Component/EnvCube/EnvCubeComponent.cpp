#include "EnvCubeComponent.h"
#include "../../Entity.h"

using namespace PaintsNow;

EnvCubeComponent::EnvCubeComponent() : range(1.0f, 1.0f, 1.0f), strength(1.0f) {}

EnvCubeComponent::~EnvCubeComponent() {}

Tiny::FLAG EnvCubeComponent::GetEntityFlagMask() const {
	return Entity::ENTITY_HAS_RENDERCONTROL | RenderableComponent::GetEntityFlagMask();
}

uint32_t EnvCubeComponent::CollectDrawCalls(std::vector<OutputRenderData>& outputDrawCalls, const InputRenderData& inputRenderData) {
	return 0;
}

TObject<IReflect>& EnvCubeComponent::operator () (IReflect& reflect) {
	BaseClass::operator () (reflect);

	if (reflect.IsReflectProperty()) {
		ReflectProperty(cubeMapTexture);
	}

	return *this;
}

void EnvCubeComponent::UpdateBoundingBox(Engine& engine, Float3Pair& box) {
	Union(box, Float3(-range));
	Union(box, range);
}
