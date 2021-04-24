#include "LightComponent.h"
#include "../Explorer/ExplorerComponent.h"
#include "../Transform/TransformComponent.h"
#include "../Visibility/VisibilityComponent.h"
#include "../../../SnowyStream/SnowyStream.h"
#include "../../../SnowyStream/Manager/RenderResourceManager.h"
#include "../../../MythForest/MythForest.h"
#include "../../../../Core/Driver/Profiler/Optick/optick.h"
#include <utility>

using namespace PaintsNow;

LightComponent::LightComponent() : attenuation(0), range(0, 0, 0) /*, spotAngle(1), temperature(6500) */ {
}

Tiny::FLAG LightComponent::GetEntityFlagMask() const {
	return Entity::ENTITY_HAS_RENDERCONTROL | RenderableComponent::GetEntityFlagMask();
}

uint32_t LightComponent::CollectDrawCalls(std::vector<OutputRenderData, DrawCallAllocator>& outputDrawCalls, const InputRenderData& inputRenderData, BytesCache& bytesCache, CollectOption option) {
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
	OPTICK_EVENT();
	for (size_t i = 0; i < shadowLayers.size(); i++) {
		TShared<ShadowLayer>& shadowLayer = shadowLayers[i];
		if (shadowLayer) {
			shadowLayer->Uninitialize(engine);
		}
	}

	BaseClass::Uninitialize(engine, entity);
}

void LightComponent::UpdateBoundingBox(Engine& engine, Float3Pair& box, bool recursive) {
	Math::Union(box, Float3(-range));
	Math::Union(box, range);
}

std::vector<TShared<LightComponent::ShadowGrid> > LightComponent::UpdateShadow(Engine& engine, const MatrixFloat4x4& cameraTransform, const MatrixFloat4x4& lightTransform, Entity* rootEntity) {
	OPTICK_EVENT();
	std::vector<TShared<ShadowGrid> > grids(shadowLayers.size());
	for (size_t i = 0; i < shadowLayers.size(); i++) {
		TShared<ShadowLayer>& shadowLayer = shadowLayers[i];
		if (shadowLayer) {
			grids[i] = shadowLayer->UpdateShadow(engine, cameraTransform, lightTransform, rootEntity);
		} 
	}

	return grids;
}

void LightComponent::BindShadowStream(Engine& engine, uint32_t layer, const TShared<StreamComponent>& streamComponent, const UShort2& res, float size, float scale) {
	OPTICK_EVENT();
	if (shadowLayers.size() <= layer) {
		shadowLayers.resize(layer + 1);
	}

	TShared<ShadowLayer>& shadowLayer = shadowLayers[layer];
	if (!shadowLayer) {
		shadowLayer = TShared<ShadowLayer>::From(new ShadowLayer(engine));
	}
	
	shadowLayer->Initialize(engine, streamComponent, res, size, scale);
}

LightComponent::ShadowLayer::ShadowLayer(Engine& engine) : gridSize(1), scale(1) {}

TShared<SharedTiny> LightComponent::ShadowLayer::StreamLoadHandler(Engine& engine, const UShort3& coord, const TShared<SharedTiny>& tiny, const TShared<SharedTiny>& context) {
	OPTICK_EVENT();
	assert(context);

	// Do nothing by now
	TShared<ShadowGrid> shadowGrid;

	if (tiny && tiny != currentGrid) {
		shadowGrid = tiny->QueryInterface(UniqueType<ShadowGrid>());
		assert(shadowGrid);
	} else {
		shadowGrid = TShared<ShadowGrid>::From(new ShadowGrid());
		shadowGrid->Flag().fetch_or(TINY_MODIFIED, std::memory_order_relaxed);
	}

	return shadowGrid();
}

void LightComponent::ShadowLayer::StreamRefreshHandler(Engine& engine, const UShort3& coord, const TShared<SharedTiny>& tiny, const TShared<SharedTiny>& context) {
	if (!(tiny->Flag().load(std::memory_order_acquire) & TINY_MODIFIED))
		return;
	
	if (!engine.snowyStream.GetRenderResourceManager()->GetCompleted())
		return;

	TShared<TaskData> taskData = currentTask;
	if (taskData->Flag().fetch_or(TINY_MODIFIED, std::memory_order_relaxed) & TINY_MODIFIED)
		return;

	if (tiny->Flag().fetch_or(TINY_UPDATING, std::memory_order_release) & TINY_UPDATING)
		return;

	TShared<ShadowGrid> shadowGrid = tiny->QueryInterface(UniqueType<ShadowGrid>());

	if (!shadowGrid->texture) {
		UShort3 dim(resolution.x(), resolution.y(), 0);
		IRender::Resource::TextureDescription depthStencilDescription;
		depthStencilDescription.dimension = dim;
		depthStencilDescription.state.format = IRender::Resource::TextureDescription::FLOAT;
		depthStencilDescription.state.layout = IRender::Resource::TextureDescription::DEPTH;

		TShared<TextureResource> texture = engine.snowyStream.CreateReflectedResource(UniqueType<TextureResource>(), "", false, ResourceBase::RESOURCE_VIRTUAL);
		texture->description.dimension = dim;
		texture->description.state.attachment = true;
		texture->description.state.format = IRender::Resource::TextureDescription::FLOAT;
		texture->description.state.layout = IRender::Resource::TextureDescription::DEPTH;
		texture->description.state.pcf = true;
		texture->Flag().fetch_or(Tiny::TINY_MODIFIED, std::memory_order_relaxed);
		texture->GetResourceManager().InvokeUpload(texture(), taskData->renderQueue);
		shadowGrid->texture = texture;
	}

	// get entity
	ShadowContext* shadowContext = context->QueryInterface(UniqueType<ShadowContext>());
	assert(shadowContext != nullptr);

	// calculate position
	CaptureData captureData;
	const MatrixFloat4x4& viewMatrix = shadowContext->lightTransformMatrix;
	OrthoCamera::UpdateCaptureData(captureData, viewMatrix);
	WorldInstanceData instanceData;
	instanceData.worldMatrix = shadowGrid->shadowMatrix = Math::QuickInverse(Math::MatrixScale(Float4(1, -1, -1, 1)) * viewMatrix);
	IRender& render = engine.interfaces.render;
	for (size_t i = 0; i < taskData->warpData.size(); i++) {
		taskData->warpData[i].bytesCache.Reset();
	}

	taskData->rootEntity = shadowContext->rootEntity; // in case of gc
	taskData->shadowGrid = shadowGrid();

	// Prepare render target
	IRender::Resource::RenderTargetDescription desc;
	desc.depthStorage.loadOp = IRender::Resource::RenderTargetDescription::CLEAR;
	desc.depthStorage.storeOp = IRender::Resource::RenderTargetDescription::DEFAULT;
	desc.depthStorage.resource = shadowGrid->texture->GetRenderResource();

	render.UploadResource(taskData->renderQueue, taskData->renderTargetResource, &desc);
	render.ExecuteResource(taskData->renderQueue, taskData->stateResource);
	render.ExecuteResource(taskData->renderQueue, taskData->renderTargetResource);

	std::atomic_thread_fence(std::memory_order_acquire);
	CollectComponentsFromEntity(engine, *taskData, instanceData, captureData, shadowContext->rootEntity());
}

TShared<SharedTiny> LightComponent::ShadowLayer::StreamUnloadHandler(Engine& engine, const UShort3& coord, const TShared<SharedTiny>& tiny, const TShared<SharedTiny>& context) {
	// tiny->Flag().fetch_or(TINY_MODIFIED, std::memory_order_relaxed);

	return nullptr;
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
	assert(r.x() > 0 && r.y() > 0 && r.z() > 0);
	range = r;
}

void LightComponent::ShadowLayer::CollectRenderableComponent(Engine& engine, TaskData& taskData, RenderableComponent* renderableComponent, TaskData::WarpData& warpData, const WorldInstanceData& instanceData) {
	IRender& render = engine.interfaces.render;
	IRender::Device* device = engine.snowyStream.GetRenderResourceManager()->GetRenderDevice();
	IDrawCallProvider::InputRenderData inputRenderData(0.0f, pipeline());
	IDrawCallProvider::DrawCallAllocator allocator(&warpData.bytesCache);
	std::vector<IDrawCallProvider::OutputRenderData, IDrawCallProvider::DrawCallAllocator> drawCalls(allocator);
	if (renderableComponent->CollectDrawCalls(drawCalls, inputRenderData, warpData.bytesCache, IDrawCallProvider::COLLECT_DEFAULT) == ~(uint32_t)0) {
		taskData.Flag().fetch_and(~TINY_ACTIVATED); // failed!!
		return;
	}

	TaskData::WarpData::InstanceGroupMap& instanceGroups = warpData.instanceGroups;

	for (size_t k = 0; k < drawCalls.size(); k++) {
		// PassBase& Pass = provider->GetPass(k);
		IDrawCallProvider::OutputRenderData& drawCall = drawCalls[k];
		const IRender::Resource::DrawCallDescription& drawCallTemplate = drawCall.drawCallDescription;
		AnimationComponent* animationComponent = instanceData.animationComponent();

		// Generate key
		InstanceKey key;
		key.renderKey = ((size_t)renderableComponent << 1) | k;
		key.animationKey = (size_t)animationComponent;

		assert((~((size_t)renderableComponent << 1) & k) == k); // assume k can be stored in renderComponent's lowest bits

		TaskData::WarpData::InstanceGroupMap::iterator it = instanceGroups.find(key);
		std::vector<IRender::Resource*> textureResources;
		std::vector<IRender::Resource::DrawCallDescription::BufferRange> bufferResources;
		InstanceGroup& group = instanceGroups[key];
		if (group.instanceCount == 0) {
			std::binary_insert(warpData.dataUpdaters, drawCall.dataUpdater);
			group.drawCallDescription = drawCallTemplate;

			std::map<ShaderResource*, TaskData::WarpData::GlobalBufferItem>::iterator ip = warpData.worldGlobalBufferMap.find(drawCall.shaderResource());
			PassBase::Updater& updater = drawCall.shaderResource->GetPassUpdater();

			if (ip == warpData.worldGlobalBufferMap.end()) {
				ip = warpData.worldGlobalBufferMap.insert(std::make_pair(drawCall.shaderResource(), TaskData::WarpData::GlobalBufferItem())).first;

				instanceData.Export(ip->second.instanceUpdater, updater);
			}

			group.instanceUpdater = &ip->second.instanceUpdater;
			group.instanceUpdater->Snapshot(group.instancedData, bufferResources, textureResources, instanceData, &warpData.bytesCache);

			// skinning
			if (animationComponent) {
				assert(animationComponent->GetWarpIndex() == renderableComponent->GetWarpIndex());
				const PassBase::Parameter& parameter = updater[IShader::BindInput::BONE_TRANSFORMS];
				if (parameter) {
					group.animationComponent = animationComponent; // hold reference
					size_t k = parameter.slot;
					IRender::Resource::DrawCallDescription::BufferRange& bufferRange =  group.drawCallDescription.bufferResources[k];
					bufferRange.buffer = animationComponent->AcquireBoneMatrixBuffer(render, warpData.renderQueue);
				}
			}
		} else {
			group.instanceUpdater->Snapshot(group.instancedData, bufferResources, textureResources, instanceData, &warpData.bytesCache);
			assert(!group.instanceUpdater->parameters.empty());
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
	OPTICK_EVENT();
	if (Flag().load(std::memory_order_acquire) & TINY_MODIFIED) {
		// engine.mythForest.StartCaptureFrame("lightdebug", "");
		std::vector<IRender::Queue*> renderQueues;
		renderQueues.emplace_back(renderQueue);
		for (size_t k = 0; k < warpData.size(); k++) {
			TaskData::WarpData& warp = warpData[k];
			renderQueues.emplace_back(warp.renderQueue);
		}

		engine.interfaces.render.SubmitQueues(&renderQueues[0], verify_cast<uint32_t>(renderQueues.size()), IRender::SUBMIT_EXECUTE_ALL);
		if (Flag().load(std::memory_order_relaxed) & TINY_ACTIVATED) {
			shadowGrid->Flag().fetch_and(~(TINY_MODIFIED | TINY_UPDATING), std::memory_order_release);
		} else {
			shadowGrid->Flag().fetch_and(~TINY_UPDATING, std::memory_order_release);
		}

		Cleanup(engine.interfaces.render);
		Flag().fetch_and(~TINY_MODIFIED, std::memory_order_release);
		// engine.mythForest.EndCaptureFrame();
	}
}

void LightComponent::ShadowLayer::CompleteCollect(Engine& engine, TaskData& task) {
	OPTICK_EVENT();

	if (task.Flag().load(std::memory_order_relaxed) & TINY_ACTIVATED) {
		// assemble 
		IRender::Queue* queue = task.renderQueue;
		IRender& render = engine.interfaces.render;

		IRender::Resource* buffer = render.CreateResource(render.GetQueueDevice(queue), IRender::Resource::RESOURCE_BUFFER);
		std::vector<IRender::Resource*> drawCallResources;

		Bytes bufferData;
		uint32_t bufferSize = 0;

		for (size_t k = 0; k < task.warpData.size(); k++) {
			TaskData::WarpData& warpData = task.warpData[k];
			for (TaskData::WarpData::InstanceGroupMap::iterator it = warpData.instanceGroups.begin(); it != warpData.instanceGroups.end(); ++it) {
				InstanceGroup& group = (*it).second;
				if (group.drawCallDescription.shaderResource == nullptr || group.instanceCount == 0) continue;

				for (size_t k = 0; k < group.instancedData.size(); k++) {
					Bytes& data = group.instancedData[k];
					assert(!data.Empty());
					if (!data.Empty()) {
						assert(data.IsViewStorage());
						// assign instanced buffer
						size_t viewSize = data.GetViewSize();
						IRender::Resource::DrawCallDescription::BufferRange& bufferRange = group.drawCallDescription.bufferResources[k];
						bufferRange.buffer = buffer;
						bufferRange.offset = bufferSize;
						bufferRange.component = verify_cast<uint8_t>(viewSize / (group.instanceCount * sizeof(float)));
						warpData.bytesCache.Link(bufferData, data); // it's safe to link different bytesCache's data for read.
						bufferSize += verify_cast<uint32_t>(viewSize);
					}
				}

				group.drawCallDescription.instanceCounts.x() = group.instanceCount;
				assert(PassBase::ValidateDrawCall(group.drawCallDescription));

				IRender::Resource* drawCall = render.CreateResource(render.GetQueueDevice(queue), IRender::Resource::RESOURCE_DRAWCALL);
				IRender::Resource::DrawCallDescription dc = group.drawCallDescription; // make copy
				render.UploadResource(queue, drawCall, &dc);
				drawCallResources.emplace_back(drawCall);
				group.Reset(); // for next reuse
			}
		}

		IRender::Resource::BufferDescription desc;
		assert(bufferSize == bufferData.GetViewSize());
		desc.data.Resize(bufferSize);
		desc.data.Import(0, bufferData);
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
	}

	engine.QueueFrameRoutine(CreateTaskContextFree(Wrap(&task, &TaskData::RenderFrame), std::ref(engine)), &task);
}

void LightComponent::ShadowLayer::CollectComponents(Engine& engine, TaskData& taskData, const WorldInstanceData& instanceData, const CaptureData& captureData, Entity* entity) {
	Tiny::FLAG rootFlag = entity->Flag().load(std::memory_order_relaxed);
	uint32_t warpIndex = entity->GetWarpIndex();
	assert(warpIndex == engine.GetKernel().GetCurrentWarpIndex());
	TaskData::WarpData& warpData = taskData.warpData[warpIndex];

	WorldInstanceData subWorldInstancedData = instanceData;
	MatrixFloat4x4 localTransform = MatrixFloat4x4::Identity();

	// has TransformComponent?
	TransformComponent* transformComponent = entity->GetUniqueComponent(UniqueType<TransformComponent>());
	bool visible = true;
	if (transformComponent != nullptr) {
		localTransform = transformComponent->GetTransform();

		// IsVisible through visibility checking?
		const Float3Pair& localBoundingBox = transformComponent->GetLocalBoundingBox();
		if ((!(transformComponent->Flag().load(std::memory_order_relaxed) & TransformComponent::TRANSFORMCOMPONENT_DYNAMIC) && !VisibilityComponent::IsVisible(captureData.visData, transformComponent)) || !captureData(localBoundingBox)) {
			visible = false;
		}

		subWorldInstancedData.worldMatrix = localTransform * instanceData.worldMatrix;
	}

	if (rootFlag & (Entity::ENTITY_HAS_RENDERABLE | Entity::ENTITY_HAS_RENDERCONTROL | Entity::ENTITY_HAS_SPACE)) {
		// optional animation
		subWorldInstancedData.animationComponent = entity->GetUniqueComponent(UniqueType<AnimationComponent>());

		ExplorerComponent::ComponentPointerAllocator allocator(&warpData.bytesCache);
		std::vector<Component*, ExplorerComponent::ComponentPointerAllocator> exploredComponents(allocator);
		ExplorerComponent* explorerComponent = entity->GetUniqueComponent(UniqueType<ExplorerComponent>());
		Component* const* componentBegin = nullptr;
		Component* const* componentEnd = nullptr;

		if (explorerComponent != nullptr) {
			// Use nearest refValue for selecting most detailed components
			explorerComponent->SelectComponents(engine, entity, 0.0f, exploredComponents);
			if (!exploredComponents.empty()) {
				componentBegin = &exploredComponents[0];
				componentEnd = componentBegin + exploredComponents.size();
			}
		} else {
			const std::vector<Component*>& components = entity->GetComponents();
			if (!components.empty()) {
				componentBegin = &components[0];
				componentEnd = componentBegin + components.size();
			}
		}

		for (Component* const* c = componentBegin; c != componentEnd; ++c) {
			Component* component = *c;
			if (component == nullptr) continue;

			if (!(component->Flag().load(std::memory_order_relaxed) & Tiny::TINY_ACTIVATED)) continue;
			Unique unique = component->GetUnique();

			// Since EntityMask would be much more faster than Reflection
			// We asserted that flaged components must be derived from specified implementations
			Tiny::FLAG entityMask = component->GetEntityFlagMask();
			// if (!(component->Flag().load(std::memory_order_relaxed) & Tiny::TINY_ACTIVATED)) continue;

			if (entityMask & Entity::ENTITY_HAS_RENDERABLE) {
				if (visible) {
					assert(component->QueryInterface(UniqueType<RenderableComponent>()) != nullptr);
					if (!(component->Flag().load(std::memory_order_relaxed) & RenderableComponent::RENDERABLECOMPONENT_CAMERAVIEW)) {
						CollectRenderableComponent(engine, taskData, static_cast<RenderableComponent*>(component), warpData, subWorldInstancedData);
					}
				}
			} else if (entityMask & Entity::ENTITY_HAS_SPACE) {
				assert(component->QueryInterface(UniqueType<SpaceComponent>()) != nullptr);
				WorldInstanceData subSpaceWorldInstancedData = subWorldInstancedData;
				subSpaceWorldInstancedData.animationComponent = nullptr; // animation info cannot be derived

				VisibilityComponent* visibilityComponent = entity->GetUniqueComponent(UniqueType<VisibilityComponent>());

				taskData.pendingCount.fetch_add(1, std::memory_order_release);

				CaptureData newCaptureData;
				const MatrixFloat4x4& mat = captureData.viewTransform;
				const Bytes& visData = visibilityComponent != nullptr ? visibilityComponent->QuerySample(engine, Float3(mat(3, 0), mat(3, 1), mat(3, 2))) : Bytes::Null();
				newCaptureData.visData = visData;
				SpaceComponent* spaceComponent = static_cast<SpaceComponent*>(component);
				bool captureFree = !!(spaceComponent->GetEntityFlagMask() & Entity::ENTITY_HAS_RENDERCONTROL);
				if (transformComponent != nullptr) {
					OrthoCamera::UpdateCaptureData(newCaptureData, mat * Math::QuickInverse(localTransform));
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
	IRender::Device* device = engine.snowyStream.GetRenderResourceManager()->GetRenderDevice();

	for (uint32_t i = 0; i < warpCount; i++) {
		warpData[i].renderQueue = render.CreateQueue(device);
	}

	renderQueue = render.CreateQueue(device);

	IRender::Resource::RenderStateDescription rs;
	rs.stencilReplacePass = 1;
	rs.cull = 1;
	rs.cullFrontFace = 1;
	rs.fill = 1;
	rs.blend = 0;
	rs.colorWrite = 0;
	rs.depthTest = IRender::Resource::RenderStateDescription::GREATER_EQUAL;
	rs.depthWrite = 1;
	rs.stencilTest = 0;
	rs.stencilWrite = 0;
	rs.stencilValue = 0;
	rs.stencilMask = 0;
	stateResource = render.CreateResource(device, IRender::Resource::RESOURCE_RENDERSTATE);
	render.UploadResource(renderQueue, stateResource, &rs);

	renderTargetResource = render.CreateResource(device, IRender::Resource::RESOURCE_RENDERTARGET);
	Flag().fetch_or(TINY_ACTIVATED, std::memory_order_release);
}

void ShadowLayerConfig::TaskData::Destroy(IRender& render) {
	Cleanup(render);
	for (size_t i = 0; i < warpData.size(); i++) {
		render.DeleteQueue(warpData[i].renderQueue);
	}

	render.DeleteResource(renderQueue, stateResource);
	render.DeleteResource(renderQueue, renderTargetResource);
	render.DeleteQueue(renderQueue);
}

void ShadowLayerConfig::TaskData::Cleanup(IRender& render) {
	rootEntity = nullptr;
	shadowGrid = nullptr;
	Flag().fetch_or(TINY_ACTIVATED, std::memory_order_relaxed);
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
void LightComponent::ShadowLayer::Initialize(Engine& engine, const TShared<StreamComponent>& component, const UShort2& res, float size, float s) {
	OPTICK_EVENT();
	Uninitialize(engine);

	streamComponent = component;
	resolution = res;
	gridSize = size;
	scale = s;

	if (streamComponent) {
		streamComponent->SetLoadHandler(Wrap(this, &ShadowLayer::StreamLoadHandler));
		streamComponent->SetRefreshHandler(Wrap(this, &ShadowLayer::StreamRefreshHandler));
		streamComponent->SetUnloadHandler(Wrap(this, &ShadowLayer::StreamUnloadHandler));
	}

	if (!pipeline) {
		String path = ShaderResource::GetShaderPathPrefix() + UniqueType<ConstMapPass>::Get()->GetBriefName();
		pipeline = engine.snowyStream.CreateReflectedResource(UniqueType<ShaderResource>(), path, true, ResourceBase::RESOURCE_VIRTUAL)->QueryInterface(UniqueType<ShaderResourceImpl<ConstMapPass> >());
	}

	TShared<TextureResource> texture = engine.snowyStream.CreateReflectedResource(UniqueType<TextureResource>(), ResourceBase::GenerateLocation("LightShadowBakeDummy", this), false, ResourceBase::RESOURCE_VIRTUAL);
	texture->description.dimension = UShort3(res.x(), res.y(), 1);
	texture->description.state.attachment = true;
	texture->description.state.format = IRender::Resource::TextureDescription::UNSIGNED_BYTE;
	texture->description.state.layout = IRender::Resource::TextureDescription::R;
	texture->description.state.pcf = true;
	texture->Flag().fetch_or(Tiny::TINY_MODIFIED, std::memory_order_relaxed);
	texture->GetResourceManager().InvokeUpload(texture(), engine.GetWarpResourceQueue());

	dummyColorAttachment = texture;

	if (!currentTask) {
		currentTask = TShared<TaskData>::From(new TaskData(engine, engine.GetKernel().GetWarpCount(), res));
	}
}

void LightComponent::ShadowLayer::Uninitialize(Engine& engine) {
	OPTICK_EVENT();
	if (streamComponent) {
		streamComponent->SetLoadHandler(nullptr);
		streamComponent->SetRefreshHandler(nullptr);
		streamComponent->SetUnloadHandler(nullptr);
	}

	if (currentTask) {
		currentTask->Destroy(engine.interfaces.render);
		currentTask = nullptr;
	}
}

TShared<LightComponent::ShadowGrid> LightComponent::ShadowLayer::UpdateShadow(Engine& engine, const MatrixFloat4x4& cameraTransform, const MatrixFloat4x4& lightTransform, Entity* rootEntity) {
	OPTICK_EVENT();
	// compute grid id
	Float3 position(cameraTransform(3, 0), cameraTransform(3, 1), cameraTransform(3, 2));

	// project to ortho plane
	Float3 lightCoord = Math::Transform(Math::QuickInverse(lightTransform), position);
	const UShort3& dimension = streamComponent->GetDimension();

	float gridScaledSize = gridSize * scale;

	Int3 intPosition((int32_t)(lightCoord.x() / gridScaledSize), (int32_t)(lightCoord.y() / gridScaledSize), (int32_t)(lightCoord.z() / gridScaledSize));
	UShort3 coord = streamComponent->ComputeWrapCoordinate(intPosition);

	TShared<ShadowContext> shadowContext = TShared<ShadowContext>::From(new ShadowContext());
	shadowContext->rootEntity = rootEntity;
	shadowContext->lightTransformMatrix = Math::MatrixScale(Float4(scale, scale, scale, 1)) * lightTransform;

	// Make alignment
	Float3 alignedPosition = Math::Transform(lightTransform, Float3(intPosition.x() * gridScaledSize, intPosition.y() * gridScaledSize, intPosition.z() * gridScaledSize));
	shadowContext->lightTransformMatrix(3, 0) = alignedPosition.x();
	shadowContext->lightTransformMatrix(3, 1) = alignedPosition.y();
	shadowContext->lightTransformMatrix(3, 2) = alignedPosition.z();
	TShared<ShadowGrid> grid;

	if (!(currentTask->Flag().load(std::memory_order_relaxed) & TINY_MODIFIED)) {
		grid = streamComponent->Load(engine, coord, shadowContext())->QueryInterface(UniqueType<ShadowGrid>());
		assert(grid);

		if (!(grid->Flag().load(std::memory_order_relaxed) & TINY_MODIFIED) || !currentGrid) {
			/*
			if (currentGrid != grid) {
				printf("Update Shadow (%.3f, %.3f, %.3f) - (%d, %d, %d) => (%d, %d, %d) [Grid Size] = %.5f [Scale] = %.5f\n", position.x(), position.y(), position.z(), intPosition.x(), intPosition.y(), intPosition.z(), coord.x(), coord.y(), coord.z(), gridScaledSize, scale);
				printf("Aligned Position (%.3f, %.3f, %.3f)\n", alignedPosition.x(), alignedPosition.y(), alignedPosition.z());
			}*/
			currentGrid = grid;
		}
	}

	return currentGrid;
}

