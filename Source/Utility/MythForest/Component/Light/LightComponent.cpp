#include "LightComponent.h"
#include "../Explorer/ExplorerComponent.h"
#include "../Transform/TransformComponent.h"
#include "../Visibility/VisibilityComponent.h"
#include "../../../SnowyStream/SnowyStream.h"

using namespace PaintsNow;
using namespace PaintsNow::NsMythForest;
using namespace PaintsNow::NsSnowyStream;


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


void LightComponent::BindShadowStream(Engine& engine, uint32_t layer, TShared<StreamComponent> streamComponent, const Float2& size) {
	if (shadowLayers.size() <= layer) {
		shadowLayers.resize(layer + 1);
	}

	TShared<ShadowLayer>& shadowLayer = shadowLayers[layer];
	if (!shadowLayer) {
		shadowLayer = TShared<ShadowLayer>::From(new ShadowLayer(engine));
	}
	
	shadowLayer->Initialize(engine, streamComponent, size);
}

LightComponent::ShadowLayer::ShadowLayer(Engine& engine) {
	taskData = TShared<TaskData>::From(new TaskData(engine, engine.GetKernel().GetWarpCount()));
}

TShared<SharedTiny> LightComponent::ShadowLayer::StreamLoadHandler(Engine& engine, const UShort3& coord, TShared<SharedTiny> tiny, TShared<SharedTiny> context) {
	assert(coord.z() == 0);
	// Do nothing by now
	TShared<ShadowGrid> shadowGrid;
	if (tiny) {
		shadowGrid = tiny->QueryInterface(UniqueType<ShadowGrid>());
		assert(shadowGrid);
	}

	// refresh shadow grid info
	shadowGrid->Flag() |= TINY_MODIFIED;

	// calculate position
	/*
	CaptureData captureData;
	MatrixFloat4x4 viewProjectionMatrix = LookAt(viewPosition, directions[face], ups[face]) * projectionMatrix;
	WorldInstanceData instanceData;
	instanceData.worldMatrix = viewProjectionMatrix;

	CollectComponentsFromEntity(engine, *taskData, instanceData, captureData, hostEntity);*/
	return shadowGrid;
}

TShared<SharedTiny> LightComponent::ShadowLayer::StreamUnloadHandler(Engine& engine, const UShort3& coord, TShared<SharedTiny> tiny, TShared<SharedTiny> context) {
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

void LightComponent::ShadowLayer::CollectRenderableComponent(Engine& engine, TaskData& taskData, RenderableComponent* renderableComponent, TaskData::WarpData& warpData, const WorldInstanceData& instanceData) {
	IRender& render = engine.interfaces.render;
	IRender::Device* device = engine.snowyStream.GetRenderDevice();
	NsSnowyStream::IDrawCallProvider::InputRenderData inputRenderData(0.0f);
	std::vector<NsSnowyStream::IDrawCallProvider::OutputRenderData> drawCalls;
	renderableComponent->CollectDrawCalls(drawCalls, inputRenderData);
	TaskData::WarpData::InstanceGroupMap& instanceGroups = warpData.instanceGroups;

	for (size_t k = 0; k < drawCalls.size(); k++) {
		// ZPassBase& Pass = provider->GetPass(k);
		NsSnowyStream::IDrawCallProvider::OutputRenderData& drawCall = drawCalls[k];
		const IRender::Resource::DrawCallDescription& drawCallTemplate = drawCall.drawCallDescription;
		AnimationComponent* animationComponent = instanceData.animationComponent();

		// Generate key
		InstanceKey key;
		key.renderKey = ((size_t)renderableComponent << 1) | k;
		key.animationKey = (size_t)animationComponent;

		assert((~((size_t)renderableComponent << 1) & k) == k); // assume k can be stored in renderComponent's lowest bits

		TaskData::WarpData::InstanceGroupMap::iterator it = instanceGroups.find(key);
		std::vector<Bytes> s;
		std::vector<IRender::Resource*> textureResources;
		std::vector<IRender::Resource::DrawCallDescription::BufferRange> bufferResources;
		InstanceGroup& group = instanceGroups[key];
		if (group.instanceCount == 0) {
			std::binary_insert(warpData.dataUpdaters, drawCall.dataUpdater);
			group.drawCallDescription = drawCallTemplate;

			std::map<ShaderResource*, TaskData::WarpData::GlobalBufferItem>::iterator ip = warpData.worldGlobalBufferMap.find(drawCall.shaderResource());
			ZPassBase::Updater& updater = drawCall.shaderResource->GetPassUpdater();

			if (ip == warpData.worldGlobalBufferMap.end()) {
				ip = warpData.worldGlobalBufferMap.insert(std::make_pair(drawCall.shaderResource(), TaskData::WarpData::GlobalBufferItem())).first;

				instanceData.Export(ip->second.instanceUpdater, updater);
			}

			for (size_t n = 0; n < group.drawCallDescription.bufferResources.size(); n++) {
				IRender::Resource::DrawCallDescription::BufferRange& bufferRange = group.drawCallDescription.bufferResources[n];
				if (ip->second.buffers[n] != nullptr) {
					bufferRange.buffer = ip->second.buffers[n];
					bufferRange.offset = bufferRange.length = 0;
				}
			}

			group.instanceUpdater = &ip->second.instanceUpdater;
			group.instanceUpdater->Snapshot(group.instancedData, bufferResources, textureResources, instanceData);

			// skinning
			if (animationComponent) {
				assert(animationComponent->GetWarpIndex() == renderableComponent->GetWarpIndex());
				ZPassBase::Parameter& parameter = updater[IShader::BindInput::BONE_TRANSFORMS];
				if (parameter) {
					group.animationComponent = animationComponent; // hold reference
					group.drawCallDescription.bufferResources[parameter.slot].buffer = animationComponent->AcquireBoneMatrixBuffer(warpData.renderQueue);
				}
			}
		} else {
			InstanceGroup& group = (*it).second;
			group.instanceUpdater->Snapshot(s, bufferResources, textureResources, instanceData);
			assert(!group.instanceUpdater->parameters.empty());
			assert(s.size() == group.instancedData.size());

			// merge slice
			for (size_t m = 0; m < group.instancedData.size(); m++) {
				group.instancedData[m].Append(s[m]);
			}
		}

		group.instanceCount++;
	}
}

void LightComponent::ShadowLayer::CompleteCollect(Engine& engine, TaskData& taskData) {}


void LightComponent::ShadowLayer::CollectComponents(Engine& engine, TaskData& taskData, const WorldInstanceData& instanceData, const CaptureData& captureData, Entity* entity) {
	Tiny::FLAG rootFlag = entity->Flag().load(std::memory_order_acquire);
	uint32_t warpIndex = entity->GetWarpIndex();
	assert(warpIndex == engine.GetKernel().GetCurrentWarpIndex());
	TaskData::WarpData& warpData = taskData.warpData[warpIndex];

	WorldInstanceData subWorldInstancedData = instanceData;
	MatrixFloat4x4 localTransform;

	// has TransformComponent?
	TransformComponent* transformComponent = entity->GetUniqueComponent(UniqueType<TransformComponent>());
	bool visible = true;
	if (transformComponent != nullptr) {
		localTransform = transformComponent->GetTransform();

		// IsVisible through visibility checking?
		if (!(transformComponent->Flag() & TransformComponent::TRANSFORMCOMPONENT_DYNAMIC) && !VisibilityComponent::IsVisible(captureData.visData, transformComponent)) {
			visible = false;
		}

		subWorldInstancedData.worldMatrix = localTransform * instanceData.worldMatrix;
	}

	if (rootFlag & (Entity::ENTITY_HAS_RENDERABLE | Entity::ENTITY_HAS_RENDERCONTROL)) {
		// optional animation
		subWorldInstancedData.animationComponent = entity->GetUniqueComponent(UniqueType<AnimationComponent>());

		std::vector<Component*> exploredComponents;
		ExplorerComponent* explorerComponent = entity->GetUniqueComponent(UniqueType<ExplorerComponent>());
		if (explorerComponent != nullptr) {
			// Use nearest refValue for selecting most detailed components
			explorerComponent->SelectComponents(engine, entity, 0.0f, exploredComponents);
		}

		const std::vector<Component*>& components = explorerComponent != nullptr ? exploredComponents : entity->GetComponents();

		for (size_t i = 0; i < components.size(); i++) {
			Component* component = components[i];
			if (component == nullptr) continue;
			Unique unique = component->GetUnique();

			// Since EntityMask would be much more faster than Reflection
			// We asserted that flaged components must be derived from specified implementations
			Tiny::FLAG entityMask = component->GetEntityFlagMask();
			assert(component->Flag() & Tiny::TINY_ACTIVATED);
			// if (!(component->Flag() & Tiny::TINY_ACTIVATED)) continue;

			if (entityMask & Entity::ENTITY_HAS_RENDERABLE) {
				if (visible) {
					assert(component->QueryInterface(UniqueType<RenderableComponent>()) != nullptr);
					CollectRenderableComponent(engine, taskData, static_cast<RenderableComponent*>(component), warpData, subWorldInstancedData);
				}
			} else if (entityMask & Entity::ENTITY_HAS_SPACE) {
				assert(component->QueryInterface(UniqueType<SpaceComponent>()) != nullptr);
				WorldInstanceData subSpaceWorldInstancedData = subWorldInstancedData;
				subSpaceWorldInstancedData.animationComponent = nullptr; // animation info cannot be derived

				VisibilityComponent* visibilityComponent = entity->GetUniqueComponent(UniqueType<VisibilityComponent>());

				++taskData.pendingCount;

				CaptureData newCaptureData;
				const MatrixFloat4x4& mat = captureData.viewTransform;
				const Bytes& visData = visibilityComponent != nullptr ? visibilityComponent->QuerySample(Float3(mat(3, 0), mat(3, 1), mat(3, 2))) : Bytes::Null();
				newCaptureData.visData = visData;
				SpaceComponent* spaceComponent = static_cast<SpaceComponent*>(component);
				bool captureFree = !!(spaceComponent->GetEntityFlagMask() & Entity::ENTITY_HAS_RENDERCONTROL);
				if (transformComponent != nullptr) {
					taskData.camera.UpdateCaptureData(newCaptureData, QuickInverse(localTransform) * mat);
					CollectComponentsFromSpace(engine, taskData, subSpaceWorldInstancedData, newCaptureData, spaceComponent);
				} else {
					CollectComponentsFromSpace(engine, taskData, subSpaceWorldInstancedData, captureData, spaceComponent);
				}
			}
		}
	}
}

LightComponent::ShadowLayer::TaskData::WarpData::WarpData() : renderQueue(nullptr) {}

LightComponent::ShadowLayer::TaskData::TaskData(Engine& engine, uint32_t warpCount) {
	warpData.resize(warpCount);
	IRender& render = engine.interfaces.render;

	for (uint32_t i = 0; i < warpCount; i++) {
		warpData[i].renderQueue = render.CreateQueue(engine.snowyStream.GetRenderDevice());
	}
}

void LightComponent::TaskData::Destroy(IRender& render) {
	for (size_t i = 0; i < warpData.size(); i++) {
		render.DeleteQueue(warpData[i].renderQueue);
	}
}

void LightComponent::TaskData::Cleanup(IRender& render) {
}


TObject<IReflect>& LightComponent::TaskData::operator () (IReflect& reflect) {
	BaseClass::operator () (reflect);

	if (reflect.IsReflectProperty()) {
	}

	return *this;
}

TObject<IReflect>& LightComponent::ShadowLayer::WorldInstanceData::operator () (IReflect& reflect) {
	BaseClass::operator () (reflect);

	if (reflect.IsReflectProperty()) {
		ReflectProperty(worldMatrix)[IShader::BindInput(IShader::BindInput::TRANSFORM_WORLD)];
	}

	return *this;
}
void LightComponent::ShadowLayer::Initialize(Engine& engine, TShared<StreamComponent> component, const Float2& size) {
	Uninitialize(engine);

	streamComponent = component;
	gridSize = size;

	if (streamComponent) {
		streamComponent->SetLoadHandler(Wrap(this, &ShadowLayer::StreamLoadHandler));
		streamComponent->SetUnloadHandler(Wrap(this, &ShadowLayer::StreamUnloadHandler));
	}
}

void LightComponent::ShadowLayer::Uninitialize(Engine& engine) {
	if (streamComponent) {
		streamComponent->SetLoadHandler(TWrapper<TShared<SharedTiny>, Engine&, const UShort3&, TShared<SharedTiny>, TShared<SharedTiny> >());
		streamComponent->SetUnloadHandler(TWrapper<TShared<SharedTiny>, Engine&, const UShort3&, TShared<SharedTiny>, TShared<SharedTiny> >());
	}
}
