#include "LightComponent.h"
#include "../Explorer/ExplorerComponent.h"
#include "../Transform/TransformComponent.h"
#include "../Visibility/VisibilityComponent.h"
#include "../../../SnowyStream/SnowyStream.h"
#include "../../../MythForest/MythForest.h"

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

void LightComponent::Uninitialize(Engine& engine, Entity* entity) {
	for (size_t i = 0; i < shadowLayers.size(); i++) {
		TShared<ShadowLayer>& shadowLayer = shadowLayers[i];
		if (shadowLayer) {
			shadowLayer->Uninitialize(engine);
		}
	}

	BaseClass::Uninitialize(engine, entity);
}

void LightComponent::UpdateBoundingBox(Engine& engine, Float3Pair& box) {
	Union(box, Float3(-range));
	Union(box, range);
}

std::vector<TShared<LightComponent::ShadowGrid> > LightComponent::UpdateShadow(Engine& engine, const MatrixFloat4x4& cameraTransform, const MatrixFloat4x4& lightTransform, Entity* rootEntity) {
	std::vector<TShared<ShadowGrid> > grids(shadowLayers.size());
	for (size_t i = 0; i < shadowLayers.size(); i++) {
		TShared<ShadowLayer>& shadowLayer = shadowLayers[i];
		if (shadowLayer) {
			grids[i] = shadowLayer->UpdateShadow(engine, cameraTransform, lightTransform, rootEntity);
		} 
	}

	return grids;
}

void LightComponent::BindShadowStream(Engine& engine, uint32_t layer, TShared<StreamComponent> streamComponent, const UShort2& res, float size, float scale) {
	if (shadowLayers.size() <= layer) {
		shadowLayers.resize(layer + 1);
	}

	TShared<ShadowLayer>& shadowLayer = shadowLayers[layer];
	if (!shadowLayer) {
		shadowLayer = TShared<ShadowLayer>::From(new ShadowLayer(engine));
	}
	
	shadowLayer->Initialize(engine, streamComponent, res, size, scale);
}

LightComponent::ShadowLayer::ShadowLayer(Engine& engine) : gridSize(1), scale(1) {
}

TShared<SharedTiny> LightComponent::ShadowLayer::StreamLoadHandler(Engine& engine, const UShort3& coord, TShared<SharedTiny> tiny, TShared<SharedTiny> context) {
	assert(coord.z() == 0);
	assert(context);

	// Do nothing by now
	TShared<ShadowGrid> shadowGrid;
	TShared<TaskData> taskData = currentTask;

	if (tiny) {
		shadowGrid = tiny->QueryInterface(UniqueType<ShadowGrid>());
		assert(shadowGrid);
	} else {
		shadowGrid = TShared<ShadowGrid>::From(new ShadowGrid());

		UShort3 dim(resolution.x(), resolution.y(), 0);
		IRender::Resource::TextureDescription depthStencilDescription;
		depthStencilDescription.dimension = dim;
		depthStencilDescription.state.format = IRender::Resource::TextureDescription::FLOAT;
		depthStencilDescription.state.layout = IRender::Resource::TextureDescription::DEPTH;

		if (!shadowGrid->texture) {
			TShared<NsSnowyStream::TextureResource> texture = engine.snowyStream.CreateReflectedResource(UniqueType<TextureResource>(), ResourceBase::GenerateLocation("LightShadowBake", shadowGrid()), false, 0, nullptr);
			texture->description.dimension = dim;
			texture->description.state.format = IRender::Resource::TextureDescription::FLOAT;
			texture->description.state.layout = IRender::Resource::TextureDescription::DEPTH;
			texture->Flag().fetch_or(Tiny::TINY_MODIFIED, std::memory_order_release);
			texture->GetResourceManager().InvokeUpload(texture(), taskData->renderQueue);
			shadowGrid->texture = texture;
		}
	}

	if (!(taskData->Flag() & TINY_MODIFIED)) {
		// refresh shadow grid info
		taskData->Flag().fetch_or(TINY_MODIFIED, std::memory_order_acquire);
		shadowGrid->Flag().fetch_or(TINY_MODIFIED, std::memory_order_acquire);

		// get entity
		ShadowContext* shadowContext = context->QueryInterface(UniqueType<ShadowContext>());
		assert(shadowContext != nullptr);

		// calculate position
		CaptureData captureData;
		MatrixFloat4x4 viewMatrix = shadowContext->lightTransformMatrix;
		viewMatrix(3, 0) = shadowContext->cameraWorldMatrix(3, 0);
		viewMatrix(3, 1) = shadowContext->cameraWorldMatrix(3, 1);
		viewMatrix(3, 2) = shadowContext->cameraWorldMatrix(3, 2);

		OrthoCamera::UpdateCaptureData(captureData, viewMatrix);
		MatrixFloat4x4 reverseDepth;

		WorldInstanceData instanceData;
		instanceData.worldMatrix = shadowGrid->shadowMatrix = QuickInverse(Scale(viewMatrix, Float4(1, -1, -1, 1)));
		taskData->rootEntity = shadowContext->rootEntity; // in case of gc
		taskData->shadowGrid = shadowGrid();
		taskData->ReferenceObject();

		// Prepare render target
		IRender::Resource::RenderTargetDescription desc;
		desc.colorBufferStorages.resize(1);
		IRender::Resource::RenderTargetDescription::Storage& s = desc.colorBufferStorages[0];
		s.resource = dummyColorAttachment->GetTexture();
		desc.depthStencilStorage.resource = shadowGrid->texture->GetTexture();

		IRender& render = engine.interfaces.render;
		render.UploadResource(taskData->renderQueue, taskData->renderTargetResource, &desc);
		render.ExecuteResource(taskData->renderQueue, taskData->renderTargetResource);
		render.ExecuteResource(taskData->renderQueue, taskData->stateResource);
		render.ExecuteResource(taskData->renderQueue, taskData->clearResource);

		CollectComponentsFromEntity(engine, *taskData, instanceData, captureData, shadowContext->rootEntity());
	}

	return shadowGrid();
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
	NsSnowyStream::IDrawCallProvider::InputRenderData inputRenderData(0.0f, pipeline());
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

			group.instanceUpdater = &ip->second.instanceUpdater;
			group.instanceUpdater->Snapshot(group.instancedData, bufferResources, textureResources, instanceData);

			// skinning
			if (animationComponent) {
				assert(animationComponent->GetWarpIndex() == renderableComponent->GetWarpIndex());
				ZPassBase::Parameter& parameter = updater[IShader::BindInput::BONE_TRANSFORMS];
				if (parameter) {
					group.animationComponent = animationComponent; // hold reference
					group.drawCallDescription.bufferResources[parameter.slot].buffer = animationComponent->AcquireBoneMatrixBuffer(render, warpData.renderQueue);
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

void LightComponent::InstanceGroup::Reset() {
	instanceCount = 0;
	for (size_t k = 0; k < instancedData.size(); k++) {
		instancedData[k].Clear();
	}
}

void ShadowLayerConfig::TaskData::RenderFrame(Engine& engine) {
	// engine.mythForest.StartCaptureFrame("lightdebug", "");
	std::vector<IRender::Queue*> renderQueues;
	renderQueues.emplace_back(renderQueue);
	for (size_t k = 0; k < warpData.size(); k++) {
		TaskData::WarpData& warp = warpData[k];
		renderQueues.emplace_back(warp.renderQueue);
	}

	engine.interfaces.render.PresentQueues(&renderQueues[0], safe_cast<uint32_t>(renderQueues.size()), IRender::PRESENT_EXECUTE_ALL);
	shadowGrid->Flag().fetch_and(~TINY_MODIFIED, std::memory_order_release);

	Flag().fetch_and(~TINY_MODIFIED, std::memory_order_release);
	Cleanup(engine.interfaces.render);
	ReleaseObject();
	// engine.mythForest.EndCaptureFrame();
}

void LightComponent::ShadowLayer::CompleteCollect(Engine& engine, TaskData& task) {
	// assemble 
	IRender::Queue* queue = task.renderQueue;
	IRender& render = engine.interfaces.render;

	IRender::Resource* buffer = render.CreateResource(queue, IRender::Resource::RESOURCE_BUFFER);
	Bytes bufferData;
	std::vector<IRender::Resource*> drawCallResources;

	for (size_t k = 0; k < task.warpData.size(); k++) {
		TaskData::WarpData& warpData = task.warpData[k];
		for (TaskData::WarpData::InstanceGroupMap::iterator it = warpData.instanceGroups.begin(); it != warpData.instanceGroups.end(); ++it) {
			InstanceGroup& group = (*it).second;
			if (group.drawCallDescription.shaderResource == nullptr || group.instanceCount == 0) continue;

			for (size_t k = 0; k < group.instancedData.size(); k++) {
				Bytes& data = group.instancedData[k];
				assert(!data.Empty());
				if (!data.Empty()) {
					ZPassBase::Parameter& output = group.instanceUpdater->parameters[k];
					// instanceable.
					assert(output.slot < group.drawCallDescription.bufferResources.size());

					// assign instanced buffer	
					IRender::Resource::DrawCallDescription::BufferRange& bufferRange = group.drawCallDescription.bufferResources[output.slot];
					bufferRange.buffer = buffer;
					bufferRange.offset = bufferData.GetSize();
					bufferRange.component = data.GetSize() / (group.instanceCount * sizeof(float));
					bufferData.Append(data);
				}
			}

			group.drawCallDescription.instanceCounts.x() = group.instanceCount;
			assert(ZPassBase::ValidateDrawCall(group.drawCallDescription));

			IRender::Resource* drawCall = render.CreateResource(queue, IRender::Resource::RESOURCE_DRAWCALL);
			IRender::Resource::DrawCallDescription dc = group.drawCallDescription; // make copy
			render.UploadResource(queue, drawCall, &dc);
			drawCallResources.emplace_back(drawCall);
			group.Reset(); // for next reuse
		}
	}

	IRender::Resource::BufferDescription desc;
	desc.data = std::move(bufferData);
	desc.format = IRender::Resource::BufferDescription::FLOAT;
	desc.usage = IRender::Resource::BufferDescription::INSTANCED;
	desc.component = 0;
	render.UploadResource(queue, buffer, &desc);

	for (size_t m = 0; m < drawCallResources.size(); m++) {
		IRender::Resource* drawCall = drawCallResources[m];
		render.ExecuteResource(queue, drawCall);
		// cleanup at current frame
		render.DeleteResource(queue, drawCall);
	}

	render.DeleteResource(queue, buffer);
	engine.QueueFrameRoutine(CreateTaskContextFree(Wrap(&task, &TaskData::RenderFrame), std::ref(engine)));
}

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
		const Float3Pair& localBoundingBox = transformComponent->GetLocalBoundingBox();
		if ((!(transformComponent->Flag() & TransformComponent::TRANSFORMCOMPONENT_DYNAMIC) && !VisibilityComponent::IsVisible(captureData.visData, transformComponent)) || !captureData(localBoundingBox)) {
			visible = false;
		}

		subWorldInstancedData.worldMatrix = localTransform * instanceData.worldMatrix;
	}

	if (rootFlag & (Entity::ENTITY_HAS_RENDERABLE | Entity::ENTITY_HAS_RENDERCONTROL | Entity::ENTITY_HAS_SPACE)) {
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
			if (!(component->Flag() & Tiny::TINY_ACTIVATED)) continue;
			Unique unique = component->GetUnique();

			// Since EntityMask would be much more faster than Reflection
			// We asserted that flaged components must be derived from specified implementations
			Tiny::FLAG entityMask = component->GetEntityFlagMask();
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

				taskData.pendingCount.fetch_add(1, std::memory_order_acquire);

				CaptureData newCaptureData;
				const MatrixFloat4x4& mat = captureData.viewTransform;
				const Bytes& visData = visibilityComponent != nullptr ? visibilityComponent->QuerySample(Float3(mat(3, 0), mat(3, 1), mat(3, 2))) : Bytes::Null();
				newCaptureData.visData = visData;
				SpaceComponent* spaceComponent = static_cast<SpaceComponent*>(component);
				bool captureFree = !!(spaceComponent->GetEntityFlagMask() & Entity::ENTITY_HAS_RENDERCONTROL);
				if (transformComponent != nullptr) {
					OrthoCamera::UpdateCaptureData(newCaptureData, mat * QuickInverse(localTransform));
					CollectComponentsFromSpace(engine, taskData, subSpaceWorldInstancedData, newCaptureData, spaceComponent);
				} else {
					CollectComponentsFromSpace(engine, taskData, subSpaceWorldInstancedData, captureData, spaceComponent);
				}
			}
		}
	}
}

ShadowLayerConfig::TaskData::WarpData::WarpData() : renderQueue(nullptr) {}

ShadowLayerConfig::TaskData::TaskData(Engine& engine, uint32_t warpCount, const UShort2& resolution) : pendingCount(0) {
	warpData.resize(warpCount);
	IRender& render = engine.interfaces.render;
	IRender::Device* device = engine.snowyStream.GetRenderDevice();

	for (uint32_t i = 0; i < warpCount; i++) {
		warpData[i].renderQueue = render.CreateQueue(device);
	}

	renderQueue = render.CreateQueue(device);

	IRender::Resource::ClearDescription cls;

	// we don't care color
	cls.clearColorBit = IRender::Resource::ClearDescription::DISCARD_LOAD | IRender::Resource::ClearDescription::DISCARD_STORE;
	cls.clearStencilBit = IRender::Resource::ClearDescription::DISCARD_LOAD | IRender::Resource::ClearDescription::DISCARD_STORE;
	cls.clearDepthBit = IRender::Resource::ClearDescription::CLEAR;

	clearResource = render.CreateResource(renderQueue, IRender::Resource::RESOURCE_CLEAR);
	render.UploadResource(renderQueue, clearResource, &cls);

	IRender::Resource::RenderStateDescription rs;
	rs.stencilReplacePass = 1;
	rs.cull = 1;
	rs.cullFrontFace = 1;
	rs.fill = 1;
	rs.alphaBlend = 0;
	rs.colorWrite = 0;
	rs.depthTest = IRender::Resource::RenderStateDescription::GREATER_EQUAL;
	rs.depthWrite = 1;
	rs.stencilTest = 0;
	rs.stencilWrite = 0;
	rs.stencilValue = 0;
	rs.stencilMask = 0;
	stateResource = render.CreateResource(renderQueue, IRender::Resource::RESOURCE_RENDERSTATE);
	render.UploadResource(renderQueue, stateResource, &rs);

	renderTargetResource = render.CreateResource(renderQueue, IRender::Resource::RESOURCE_RENDERTARGET);
}

void ShadowLayerConfig::TaskData::Destroy(IRender& render) {
	for (size_t i = 0; i < warpData.size(); i++) {
		render.DeleteQueue(warpData[i].renderQueue);
	}

	render.DeleteResource(renderQueue, stateResource);
	render.DeleteResource(renderQueue, clearResource);
	render.DeleteResource(renderQueue, renderTargetResource);
	render.DeleteQueue(renderQueue);
}

void ShadowLayerConfig::TaskData::Cleanup(IRender& render) {
	rootEntity = nullptr;
	shadowGrid = nullptr;
}

TObject<IReflect>& ShadowLayerConfig::TaskData::operator () (IReflect& reflect) {
	BaseClass::operator () (reflect);

	if (reflect.IsReflectProperty()) {
	}

	return *this;
}

TObject<IReflect>& ShadowLayerConfig::WorldInstanceData::operator () (IReflect& reflect) {
	BaseClass::operator () (reflect);

	if (reflect.IsReflectProperty()) {
		ReflectProperty(worldMatrix)[IShader::BindInput(IShader::BindInput::TRANSFORM_WORLD)];
		ReflectProperty(instancedColor)[IShader::BindInput(IShader::BindInput::COLOR_INSTANCED)];
	}

	return *this;
}
void LightComponent::ShadowLayer::Initialize(Engine& engine, TShared<StreamComponent> component, const UShort2& res, float size, float s) {
	Uninitialize(engine);

	streamComponent = component;
	resolution = res;
	gridSize = size;
	scale = s;

	if (streamComponent) {
		streamComponent->SetLoadHandler(Wrap(this, &ShadowLayer::StreamLoadHandler));
		streamComponent->SetUnloadHandler(Wrap(this, &ShadowLayer::StreamUnloadHandler));
	}

	if (!pipeline) {
		String path = NsSnowyStream::ShaderResource::GetShaderPathPrefix() + UniqueType<ConstMapPass>::Get()->GetSubName();
		pipeline = engine.snowyStream.CreateReflectedResource(UniqueType<ShaderResource>(), path, true, 0, nullptr)->QueryInterface(UniqueType<ShaderResourceImpl<ConstMapPass> >());
	}

	TShared<NsSnowyStream::TextureResource> texture = engine.snowyStream.CreateReflectedResource(UniqueType<TextureResource>(), ResourceBase::GenerateLocation("LightShadowBakeDummy", this), false, 0, nullptr);
	texture->description.dimension = UShort3(res.x(), res.y(), 1);
	texture->description.state.format = IRender::Resource::TextureDescription::UNSIGNED_BYTE;
	texture->description.state.layout = IRender::Resource::TextureDescription::R;
	texture->Flag().fetch_or(Tiny::TINY_MODIFIED, std::memory_order_release);
	texture->GetResourceManager().InvokeUpload(texture(), engine.mythForest.GetWarpResourceQueue());

	dummyColorAttachment = texture;

	if (!currentTask) {
		currentTask = TShared<TaskData>::From(new TaskData(engine, engine.GetKernel().GetWarpCount(), res));
	}
}

void LightComponent::ShadowLayer::Uninitialize(Engine& engine) {
	if (streamComponent) {
		streamComponent->SetLoadHandler(TWrapper<TShared<SharedTiny>, Engine&, const UShort3&, TShared<SharedTiny>, TShared<SharedTiny> >());
		streamComponent->SetUnloadHandler(TWrapper<TShared<SharedTiny>, Engine&, const UShort3&, TShared<SharedTiny>, TShared<SharedTiny> >());
	}

	if (currentTask) {
		currentTask->Destroy(engine.interfaces.render);
		currentTask = nullptr;
	}
}

TShared<LightComponent::ShadowGrid> LightComponent::ShadowLayer::UpdateShadow(Engine& engine, const MatrixFloat4x4& cameraTransform, const MatrixFloat4x4& lightTransform, Entity* rootEntity) {
	// compute grid id
	Float3 position(cameraTransform(3, 0), cameraTransform(3, 1), cameraTransform(3, 2));

	// project to ortho plane
	Float3 lightCoord = Transform3D(QuickInverse(lightTransform), position);
	const UShort3& dimension = streamComponent->GetDimension();

	UShort3 coord(
		safe_cast<uint16_t>((int(lightCoord.x() / gridSize) % dimension.x() + dimension.x()) % dimension.x()),
		safe_cast<uint16_t>((int(lightCoord.y() / gridSize) % dimension.y() + dimension.y()) % dimension.y()),
		0);

	TShared<ShadowContext> shadowContext = TShared<ShadowContext>::From(new ShadowContext());
	shadowContext->rootEntity = rootEntity;
	shadowContext->cameraWorldMatrix = cameraTransform;
	shadowContext->lightTransformMatrix = Scale(lightTransform, Float4(scale, scale, scale, 1));
	TShared<ShadowGrid> grid = streamComponent->Load(engine, coord, shadowContext())->QueryInterface(UniqueType<ShadowGrid>());
	assert(grid);
	// printf("COORD: %d, %d, %d\n", coord.x(), coord.y(), coord.z());
	return grid;
}

