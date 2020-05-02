#include "LightComponent.h"

using namespace PaintsNow;
using namespace PaintsNow::NsMythForest;

LightComponent::LightComponent() : attenuation(0) /*, spotAngle(1), temperature(6500) */ {
}

Tiny::FLAG LightComponent::GetEntityFlagMask() const {
	return Entity::ENTITY_HAS_RENDERCONTROL | RenderableComponent::GetEntityFlagMask();
}

uint32_t LightComponent::CollectDrawCalls(std::vector<OutputRenderData>& outputDrawCalls, const InputRenderData& inputRenderData) {
	return 0;
}

TObject<IReflect>& LightComponent::operator () (IReflect& reflect) {
	BaseClass::operator () (reflect);

	if (reflect.IsReflectProperty()) {
		ReflectProperty(color);
		ReflectProperty(attenuation);
		/*
		ReflectProperty(spotAngle);
		ReflectProperty(temperature);*/
	}

	return *this;
}

void LightComponent::UpdateBoundingBox(Engine& engine, Float3Pair& box) {
	Union(box, Float3(-range));
	Union(box, range);
}

void LightComponent::ReplaceStreamComponent(ShadowLayer& shadowLayer, TShared<StreamComponent> streamComponent) {
	if (shadowLayer.streamComponent) {
		shadowLayer.streamComponent->SetLoadHandler(TWrapper<TShared<SharedTiny>, Engine&, const UShort3&, TShared<SharedTiny> >());
		shadowLayer.streamComponent->SetUnloadHandler(TWrapper<TShared<SharedTiny>, Engine&, const UShort3&, TShared<SharedTiny> >());
	}

	shadowLayer.streamComponent = streamComponent;

	if (streamComponent) {
		shadowLayer.streamComponent->SetLoadHandler(Wrap(this, &LightComponent::StreamLoadHandler));
		shadowLayer.streamComponent->SetUnloadHandler(Wrap(this, &LightComponent::StreamUnloadHandler));
	}
}

void LightComponent::BindShadowStream(uint32_t layer, TShared<StreamComponent> streamComponent, const Float2& size) {
	if (shadowLayers.size() <= layer) {
		shadowLayers.resize(layer + 1);
	}

	ShadowLayer& shadowLayer = shadowLayers[layer];
	ReplaceStreamComponent(shadowLayer, streamComponent);
	shadowLayer.gridSize = size;
}

void LightComponent::Uninitialize(Engine& engine, Entity* entity) {
	for (size_t i = 0; i < shadowLayers.size(); i++) {
		ReplaceStreamComponent(shadowLayers[i], nullptr);
	}

	BaseClass::Uninitialize(engine, entity);
}

TShared<SharedTiny> LightComponent::StreamLoadHandler(Engine& engine, const UShort3& coord, TShared<SharedTiny> tiny) {
	// Do nothing by now
	TShared<ShadowGrid> shadowGrid;
	if (tiny) {
		shadowGrid = tiny->QueryInterface(UniqueType<ShadowGrid>());
		assert(shadowGrid);
	}

	// refresh shadow grid info
	return nullptr;
}

TShared<SharedTiny> LightComponent::StreamUnloadHandler(Engine& engine, const UShort3& coord, TShared<SharedTiny> tiny) {
	return tiny;
}

const Float3& LightComponent::GetColor() const {
	return color;
}

void LightComponent::SetColor(const Float3& c) {
	color = c;
}

float LightComponent::GetAttenuation() const {
	return attenuation;
}

void LightComponent::SetAttenuation(float value) {
	attenuation = value;
}

const Float3& LightComponent::GetRange() const {
	return range;
}

void LightComponent::SetRange(const Float3& r) {
	range = r;
}

