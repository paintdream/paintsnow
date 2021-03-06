#include "VisibilityComponent.h"
#include "../../../SnowyStream/SnowyStream.h"
#include "../../../SnowyStream/Manager/RenderResourceManager.h"
#include "../Space/SpaceComponent.h"
#include "../Renderable/RenderableComponent.h"
#include "../Transform/TransformComponent.h"
#include "../Explorer/ExplorerComponent.h"
#include "../../../../Core/Interface/IMemory.h"
#include "../../../../Core/Driver/Profiler/Optick/optick.h"
#include <cmath>

using namespace PaintsNow;

VisibilityComponent::VisibilityComponent(const TShared<StreamComponent>& stream) : gridSize(0.5f, 0.5f, 0.5f), viewDistance(512.0f), taskCount(32), resolution(128, 128), activeCellCacheIndex(0), hostEntity(nullptr), renderQueue(nullptr), depthStencilResource(nullptr), stateResource(nullptr), collectLock(nullptr), streamComponent(stream) {
	maxVisIdentity.store(0, std::memory_order_relaxed);
	cellAllocator.Reset(new TObjectAllocator<Cell>());
}

Tiny::FLAG VisibilityComponent::GetEntityFlagMask() const {
	return Entity::ENTITY_HAS_TICK_EVENT | Entity::ENTITY_HAS_SPECIAL_EVENT;
}

TObject<IReflect>& VisibilityComponent::operator () (IReflect& reflect) {
	BaseClass::operator () (reflect);

	if (reflect.IsReflectProperty()) {
		ReflectProperty(gridSize);
		ReflectProperty(viewDistance);
		ReflectProperty(resolution);
		ReflectProperty(taskCount);
	}

	return *this;
}

void VisibilityComponent::Initialize(Engine& engine, Entity* entity) {
	OPTICK_EVENT();
	assert(hostEntity == nullptr);
	assert(renderQueue == nullptr);
	hostEntity = entity;

	// Allocate IDs
	maxVisIdentity.store(1, std::memory_order_relaxed);

	const std::vector<Component*>& components = hostEntity->GetComponents();
	for (size_t n = 0; n < components.size(); n++) {
		Component* component = components[n];
		if (component != nullptr && (component->GetEntityFlagMask() & Entity::ENTITY_HAS_SPACE)) {
			SpaceComponent* spaceComponent = static_cast<SpaceComponent*>(component);
			SetupIdentities(engine, spaceComponent->GetRootEntity());
		}
	}

	IThread& thread = engine.interfaces.thread;
	collectLock = thread.NewLock();

	IRender& render = engine.interfaces.render;
	IRender::Device* device = engine.snowyStream.GetRenderResourceManager()->GetRenderDevice();
	renderQueue = render.CreateQueue(device);

	String path = ShaderResource::GetShaderPathPrefix() + UniqueType<ConstMapPass>::Get()->GetBriefName();
	pipeline = engine.snowyStream.CreateReflectedResource(UniqueType<ShaderResource>(), path, true, ResourceBase::RESOURCE_VIRTUAL)->QueryInterface(UniqueType<ShaderResourceImpl<ConstMapPass> >());

	IRender::Resource::RenderStateDescription rs;
	rs.stencilReplacePass = 1;
	rs.cull = 0;
	rs.fill = 1;
	rs.blend = 0;
	rs.colorWrite = 1;
	rs.depthTest = IRender::Resource::RenderStateDescription::GREATER_EQUAL;
	rs.depthWrite = 1;
	rs.stencilTest = 0;
	rs.stencilWrite = 0;
	rs.stencilValue = 0;
	rs.stencilMask = 0;
	stateResource = render.CreateResource(device, IRender::Resource::RESOURCE_RENDERSTATE);
	render.UploadResource(renderQueue, stateResource, &rs);

	UShort3 dim(resolution.x(), resolution.y(), 0);
	IRender::Resource::TextureDescription depthStencilDescription;
	depthStencilDescription.dimension = dim;
	depthStencilDescription.state.format = IRender::Resource::TextureDescription::FLOAT;
	depthStencilDescription.state.layout = IRender::Resource::TextureDescription::DEPTH_STENCIL;
	depthStencilResource = render.CreateResource(device, IRender::Resource::RESOURCE_TEXTURE);
	render.UploadResource(renderQueue, depthStencilResource, &depthStencilDescription);

	tasks.resize(taskCount);

	for (size_t i = 0; i < tasks.size(); i++) {
		TaskData& task = tasks[i];
		task.renderQueue = render.CreateQueue(device);

		TShared<TextureResource>& texture = task.texture;
		texture = engine.snowyStream.CreateReflectedResource(UniqueType<TextureResource>(), ResourceBase::GenerateLocation("VisBake", &task), false, ResourceBase::RESOURCE_VIRTUAL);
		texture->description.dimension = dim;
		texture->description.state.attachment = true;
		texture->description.state.format = IRender::Resource::TextureDescription::UNSIGNED_BYTE;
		texture->description.state.layout = IRender::Resource::TextureDescription::RGBA;
		texture->Flag().fetch_or(Tiny::TINY_MODIFIED, std::memory_order_release);
		texture->GetResourceManager().InvokeUpload(texture(), renderQueue);

		IRender::Resource::RenderTargetDescription desc;
		desc.colorStorages.resize(1);
		IRender::Resource::RenderTargetDescription::Storage& s = desc.colorStorages[0];
		s.resource = texture->GetRenderResource();
		s.loadOp = IRender::Resource::RenderTargetDescription::CLEAR;
		s.storeOp = IRender::Resource::RenderTargetDescription::DEFAULT;
		desc.depthStorage.resource = depthStencilResource;
		desc.depthStorage.loadOp = IRender::Resource::RenderTargetDescription::CLEAR;
		desc.depthStorage.storeOp = IRender::Resource::RenderTargetDescription::DISCARD;

		task.renderTarget = render.CreateResource(device, IRender::Resource::RESOURCE_RENDERTARGET);
		render.UploadResource(renderQueue, task.renderTarget, &desc);
		task.warpData.resize(engine.GetKernel().GetWarpCount());
	}

	streamComponent->SetLoadHandler(Wrap(this, &VisibilityComponent::StreamLoadHandler));
	streamComponent->SetUnloadHandler(Wrap(this, &VisibilityComponent::StreamUnloadHandler));

	BaseComponent::Initialize(engine, entity);
}

void VisibilityComponent::Uninitialize(Engine& engine, Entity* entity) {
	OPTICK_EVENT();
	// TODO: wait for all download tasks to finish.
	assert(hostEntity != nullptr);
	streamComponent->SetLoadHandler(TWrapper<TShared<SharedTiny>, Engine&, const UShort3&, const TShared<SharedTiny>&, const TShared<SharedTiny>&>());
	streamComponent->SetUnloadHandler(TWrapper<TShared<SharedTiny>, Engine&, const UShort3&, const TShared<SharedTiny>&, const TShared<SharedTiny>&>());

	hostEntity = nullptr;
	IRender& render = engine.interfaces.render;
	for (size_t k = 0; k < tasks.size(); k++) {
		TaskData& task = tasks[k];
		render.DeleteResource(task.renderQueue, task.renderTarget);
		render.DeleteQueue(task.renderQueue);
	}

	tasks.clear();

	render.DeleteResource(renderQueue, stateResource);
	render.DeleteResource(renderQueue, depthStencilResource);
	render.DeleteQueue(renderQueue);

	stateResource = depthStencilResource = nullptr;
	renderQueue = nullptr;

	engine.interfaces.thread.DeleteLock(collectLock);
	collectLock = nullptr;
	BaseComponent::Uninitialize(engine, entity);
}

Entity* VisibilityComponent::GetHostEntity() const {
	return hostEntity;
}

void VisibilityComponent::DispatchEvent(Event& event, Entity* entity) {
	OPTICK_EVENT();
	if (event.eventID == Event::EVENT_FRAME) {
		// DoBake
		TickRender(event.engine);

		// Prepare
		Engine& engine = event.engine;
		engine.GetKernel().QueueRoutine(this, CreateTaskContextFree(Wrap(this, &VisibilityComponent::RoutineTickTasks), std::ref(engine)));
	}
}

void VisibilityComponent::Setup(Engine& engine, float distance, const Float3& size, uint32_t tc, const UShort2& res) {
	assert(!(Flag().load(std::memory_order_acquire) & TINY_MODIFIED));

	taskCount = tc;
	gridSize = size;
	viewDistance = distance;
	resolution = res;

	Flag().fetch_or(TINY_MODIFIED, std::memory_order_release);
}

void VisibilityComponent::SetupIdentities(Engine& engine, Entity* entity) {
	for (Entity* p = entity; p != nullptr; p = p->Right()) {
		TransformComponent* transformComponent = p->GetUniqueComponent(UniqueType<TransformComponent>());
		if (transformComponent != nullptr) {
			assert(transformComponent->GetObjectID() == 0);
			transformComponent->SetObjectID(maxVisIdentity.fetch_add(1));
		}

		const std::vector<Component*>& components = p->GetComponents();
		for (size_t i = 0; i < components.size(); i++) {
			Component* component = components[i];
			if (component != nullptr && (component->GetEntityFlagMask() & Entity::ENTITY_HAS_SPACE)) {
				assert(component->GetWarpIndex() == GetWarpIndex());
				SetupIdentities(engine, static_cast<SpaceComponent*>(component)->GetRootEntity());
			}
		}

		if (p->Left() != nullptr) {
			SetupIdentities(engine, p->Left());
		}
	}
}

static inline void MergeSample(Bytes& dst, const Bytes& src) {
	size_t s = src.GetSize();
	const uint8_t* f = src.GetData();
	uint8_t* t = dst.GetData();

	for (uint32_t i = 0; i < s; i++) {
		t[i] |= f[i];
	}
}

VisibilityComponentConfig::Cell::Cell() {
	dispatched.store(0, std::memory_order_relaxed);
	finished.store(0, std::memory_order_relaxed);
	taskHead.store(nullptr, std::memory_order_release);
}

const Bytes& VisibilityComponent::QuerySample(Engine& engine, const Float3& position) {
	OPTICK_EVENT();
	Int3 intPosition(int32_t(position.x() / gridSize.x()), int32_t(position.y() / gridSize.y()), int32_t(position.z() / gridSize.z()));
	const UShort3& dimension = streamComponent->GetDimension();
	assert(dimension.x() >= 4 && dimension.y() >= 4 && dimension.z() >= 4);
	assert(streamComponent->GetCacheCount() >= 64);

	// load cache
	const uint32_t cacheCount = sizeof(cellCache) / sizeof(cellCache[0]);
	for (uint32_t i = activeCellCacheIndex; i < cacheCount + activeCellCacheIndex; i++) {
		Cache& current = cellCache[i % cacheCount];
		if (!current.payload.Empty() && current.intPosition == intPosition) {
			// hit!
			return current.payload;
		}
	}

	std::vector<TShared<Cell> > toMerge;
	toMerge.reserve(27);
	uint32_t mergedSize = 0;
	bool incomplete = false;

	for (int32_t z = intPosition.z() - 1; z <= intPosition.z() + 1; z++) {
		for (int32_t y = intPosition.y() - 1; y <= intPosition.y() + 1; y++) {
			for (int32_t x = intPosition.x() - 1; x <= intPosition.x() + 1; x++) {
				Int3 coord(x, y, z);
				TShared<Cell> cell = streamComponent->Load(engine, streamComponent->ComputeWrapCoordinate(coord), nullptr)->QueryInterface(UniqueType<Cell>());

				bool curIncomplete = cell->finished.load(std::memory_order_acquire) != Cell::ALL_FACE_MASK;
				if (curIncomplete) {
					cell->intPosition = coord;
					std::binary_insert(bakePoints, cell);
				}

				incomplete = incomplete || curIncomplete;
				toMerge.emplace_back(cell);
				mergedSize = Math::Max(mergedSize, (uint32_t)verify_cast<uint32_t>(cell->payload.GetSize()));
			}
		}
	}

	if (incomplete) return Bytes::Null();

	// Do Merge
	Bytes mergedPayload;
	mergedPayload.Resize(mergedSize, 0);
	for (size_t j = 0; j < toMerge.size(); j++) {
		MergeSample(mergedPayload, toMerge[j]->payload);
	}

	Cache& targetCache = cellCache[activeCellCacheIndex];
	targetCache.intPosition = intPosition;
	targetCache.payload = std::move(mergedPayload);
	activeCellCacheIndex = (activeCellCacheIndex + 1) % (sizeof(cellCache) / sizeof(cellCache[0]));
	return targetCache.payload;
}

TShared<SharedTiny> VisibilityComponent::StreamLoadHandler(Engine& engine, const UShort3& coord, const TShared<SharedTiny>& tiny, const TShared<SharedTiny>& context) {
	OPTICK_EVENT();
	return tiny ? tiny->QueryInterface(UniqueType<Cell>()) : TShared<SharedTiny>::From(cellAllocator->New());
}

TShared<SharedTiny> VisibilityComponent::StreamUnloadHandler(Engine& engine, const UShort3& coord, const TShared<SharedTiny>& tiny, const TShared<SharedTiny>& context) {
	TShared<Cell> cell = tiny->QueryInterface(UniqueType<Cell>());

	// is ready?
	if (cell->finished.load(std::memory_order_acquire) == Cell::ALL_FACE_MASK) {
		cell->payload.Clear();
		cell->dispatched.store(0, std::memory_order_release);
		cell->finished.store(0, std::memory_order_release);
		cell->Flag().fetch_and(~TINY_ACTIVATED, std::memory_order_release);

		return cell();
	} else {
		return nullptr; // let it choose a new one
	}
}

bool VisibilityComponent::IsVisible(const Bytes& s, TransformComponent* transformComponent) {
	if (s.Empty()) return true;
	uint32_t id = transformComponent->GetObjectID();

	uint32_t offset = id >> 3;
	size_t size = s.GetSize();
	if (offset >= size) return false;
	const uint8_t* ptr = s.GetData();
	uint8_t testBit = 1 << (id & 7);

	assert(offset < size);
	return !!(ptr[offset] & testBit);
}

// Baker

void VisibilityComponent::TickRender(Engine& engine) {
	OPTICK_EVENT();
	// check pipeline state
	if (renderQueue == nullptr) return; // not inited.
	IRender& render = engine.interfaces.render;
	render.SubmitQueues(&renderQueue, 1, IRender::SUBMIT_EXECUTE_ALL);

	// read previous results back and collect ready ones
	std::vector<IRender::Queue*> bakeQueues;
	for (size_t i = 0; i < tasks.size(); i++) {
		TaskData& task = tasks[i];
		TextureResource* texture = task.texture();
		std::atomic<uint32_t>& finalStatus = reinterpret_cast<std::atomic<uint32_t>&>(task.status);
		if (task.status == TaskData::STATUS_IDLE) {
			finalStatus.store(TaskData::STATUS_START, std::memory_order_release);
		} else if (task.status == TaskData::STATUS_ASSEMBLED) {
			if (task.Continue()) {
				// printf("REQUEST %p\n", texture->GetRenderResource());
				render.RequestDownloadResource(task.renderQueue, texture->GetRenderResource(), &texture->description);
				render.FlushQueue(task.renderQueue);
				bakeQueues.emplace_back(task.renderQueue);
				finalStatus.store(TaskData::STATUS_BAKING, std::memory_order_release);
			} else {
				// failed!!
				task.cell->dispatched.fetch_and(~(1 << task.faceIndex), std::memory_order_relaxed);
				finalStatus.store(TaskData::STATUS_IDLE, std::memory_order_release);
			}
		} else if (task.status == TaskData::STATUS_BAKING) {
			render.CompleteDownloadResource(task.renderQueue, texture->GetRenderResource());
			bakeQueues.emplace_back(task.renderQueue);
			task.data = std::move(texture->description.data);
			finalStatus.store(TaskData::STATUS_BAKED, std::memory_order_release);
		}
	}

	// Commit bakes
	if (!bakeQueues.empty()) {
		render.SubmitQueues(&bakeQueues[0], verify_cast<uint32_t>(bakeQueues.size()), IRender::SUBMIT_EXECUTE);
	}
}

TObject<IReflect>& VisibilityComponent::WorldInstanceData::operator () (IReflect& reflect) {
	BaseClass::operator () (reflect);

	if (reflect.IsReflectProperty()) {
		ReflectProperty(worldMatrix)[IShader::BindInput(IShader::BindInput::TRANSFORM_WORLD)];
		ReflectProperty(instancedColor)[IShader::BindInput(IShader::BindInput::COLOR_INSTANCED)];
	}

	return *this;
}

void VisibilityComponent::InstanceGroup::Reset() {
	instanceCount = 0;
	for (size_t k = 0; k < instancedData.size(); k++) {
		instancedData[k].Clear();
	}
}

void VisibilityComponent::CollectRenderableComponent(Engine& engine, TaskData& task, RenderableComponent* renderableComponent, WorldInstanceData& instanceData, uint32_t identity) {
	IRender& render = engine.interfaces.render;
	IRender::Queue* queue = task.renderQueue;
	const UChar4& encode = *reinterpret_cast<const UChar4*>(&identity);
	instanceData.instancedColor = Float4((float)encode[0], (float)encode[1], (float)encode[2], (float)encode[3]) / 255.0f;
	uint32_t currentWarpIndex = engine.GetKernel().GetCurrentWarpIndex();
	TaskData::WarpData& warpData = task.warpData[currentWarpIndex == ~(uint32_t)0 ? GetWarpIndex() : currentWarpIndex];
	std::unordered_map<size_t, InstanceGroup>& instanceGroups = warpData.instanceGroups;
	InstanceGroup& first = instanceGroups[(size_t)renderableComponent];

	std::vector<IRender::Resource*> textureResources;
	std::vector<IRender::Resource::DrawCallDescription::BufferRange> bufferResources;

	IThread& thread = engine.interfaces.thread;
	if (first.drawCallDescription.shaderResource != nullptr) {
		size_t i = 0;
		while (true) {
			InstanceGroup& group = instanceGroups[(size_t)renderableComponent + i++];
			if (group.drawCallDescription.shaderResource == nullptr) break;

			thread.DoLock(collectLock);
			group.instanceUpdater.Snapshot(group.instancedData, bufferResources, textureResources, instanceData, &warpData.bytesCache);
			thread.UnLock(collectLock);
			group.instanceCount++;
		}
	} else {
		IDrawCallProvider::InputRenderData inputRenderData(0.0f, pipeline());
		IDrawCallProvider::DrawCallAllocator allocator(&warpData.bytesCache);
		std::vector<IDrawCallProvider::OutputRenderData, IDrawCallProvider::DrawCallAllocator> drawCalls(allocator);
		thread.DoLock(collectLock);
		uint32_t count = renderableComponent->CollectDrawCalls(drawCalls, inputRenderData, warpData.bytesCache, IDrawCallProvider::COLLECT_DEFAULT);

		if (count == ~(uint32_t)0) { // failed?
			task.pendingResourceCount++;
		}

		thread.UnLock(collectLock);

		if (task.pendingResourceCount != 0)
			return;

		assert(drawCalls.size() < sizeof(RenderableComponent) - 1);

		for (size_t i = 0; i < drawCalls.size(); i++) {
			assert(drawCalls[i].shaderResource->Flag().load(std::memory_order_relaxed) & ResourceBase::RESOURCE_UPLOADED);
			IDataUpdater* dataUpdater = drawCalls[i].dataUpdater;
			InstanceGroup& group = instanceGroups[(size_t)renderableComponent + i];
			if (group.instanceCount == 0) {
				std::binary_insert(task.dataUpdaters, dataUpdater);

				instanceData.Export(group.instanceUpdater, pipeline->GetPassUpdater());
				group.drawCallDescription = std::move(drawCalls[i].drawCallDescription);
			}

			group.instanceUpdater.Snapshot(group.instancedData, bufferResources, textureResources, instanceData, &warpData.bytesCache);
			group.instanceCount++;
		}
	}
}

void VisibilityComponent::CompleteCollect(Engine& engine, TaskData& task) {}

void VisibilityComponent::CollectComponents(Engine& engine, TaskData& task, const WorldInstanceData& inst, const CaptureData& captureData, Entity* entity) {
	TransformComponent* transformComponent = entity->GetUniqueComponent(UniqueType<TransformComponent>());
	if (transformComponent != nullptr) {
		const Float3Pair& localBoundingBox = transformComponent->GetLocalBoundingBox();
		if (!captureData(localBoundingBox))
			return;
	}

	WorldInstanceData instanceData;
	instanceData.worldMatrix = transformComponent != nullptr ? transformComponent->GetTransform() * inst.worldMatrix : inst.worldMatrix;

	uint32_t currentWarpIndex = engine.GetKernel().GetCurrentWarpIndex();
	TaskData::WarpData& warpData = task.warpData[currentWarpIndex == ~(uint32_t)0 ? GetWarpIndex() : currentWarpIndex];
	ExplorerComponent::ComponentPointerAllocator allocator(&warpData.bytesCache);
	std::vector<Component*, ExplorerComponent::ComponentPointerAllocator> exploredComponents(allocator);
	ExplorerComponent* explorerComponent = entity->GetUniqueComponent(UniqueType<ExplorerComponent>());
	Component* const* componentBegin = nullptr;
	Component* const* componentEnd = nullptr;

	static Unique expectedIdentifier = UniqueType<RenderableComponent>::Get();
	if (explorerComponent != nullptr && explorerComponent->GetExploreIdentifier() == expectedIdentifier) {
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

		if (component->GetEntityFlagMask() & Entity::ENTITY_HAS_RENDERABLE) {
			if (transformComponent != nullptr) {
				RenderableComponent* renderableComponent = static_cast<RenderableComponent*>(component);
				if (!(renderableComponent->Flag().load(std::memory_order_acquire) & RenderableComponent::RENDERABLECOMPONENT_CAMERAVIEW)) {
					CollectRenderableComponent(engine, task, renderableComponent, instanceData, transformComponent->GetObjectID());
				}
			}
		} else if (component->GetEntityFlagMask() & Entity::ENTITY_HAS_SPACE) {
			std::atomic<uint32_t>& counter = reinterpret_cast<std::atomic<uint32_t>&>(task.pendingCount);
			counter.fetch_add(1, std::memory_order_acquire);
			SpaceComponent* spaceComponent = static_cast<SpaceComponent*>(component);
			if (transformComponent != nullptr) {
				CaptureData newCaptureData;
				task.camera.UpdateCaptureData(newCaptureData, captureData.viewTransform * Math::QuickInverse(transformComponent->GetTransform()));
				CollectComponentsFromSpace(engine, task, instanceData, newCaptureData, spaceComponent);
			} else {
				CollectComponentsFromSpace(engine, task, instanceData, captureData, spaceComponent);
			}
		}
	}
}

void VisibilityComponent::PostProcess(const TShared<Cell>& cell) {
	TaskData* dataList;
	while ((dataList = cell->taskHead.exchange(nullptr, std::memory_order_release)) != nullptr) {
		OPTICK_EVENT();
		while (dataList != nullptr) {
			TaskData& task = *dataList;
			assert(task.status == TaskData::STATUS_POSTPROCESS);
			Cell& cell = *task.cell;
			Bytes& encodedData = cell.payload;
			const Bytes& data = task.data;
			assert(data.GetSize() % sizeof(uint32_t) == 0);
			uint32_t count = verify_cast<uint32_t>(data.GetSize()) / sizeof(uint32_t);
			const uint32_t* p = reinterpret_cast<const uint32_t*>(data.GetData());
			uint8_t* target = encodedData.GetData();

			for (size_t m = 0; m < count; m++) {
				uint32_t objectID = p[m];
				uint32_t location = objectID >> 3;
				if (location >= encodedData.GetSize()) {
					encodedData.Resize(location + 1, 0);
					target = encodedData.GetData();
				}

				target[location] |= 1 << (objectID & 7);
			}

			dataList = dataList->next;
			task.next = nullptr;
			cell.finished.fetch_or(1 << task.faceIndex, std::memory_order_release);

			// cell finished?
			std::atomic<uint32_t>& finalStatus = reinterpret_cast<std::atomic<uint32_t>&>(task.status);
			finalStatus.store(TaskData::STATUS_IDLE, std::memory_order_release);
		}
	}
}

void VisibilityComponent::ResolveTasks(Engine& engine) {
	OPTICK_EVENT();
	// resolve finished tasks
	for (size_t k = 0; k < tasks.size(); k++) {
		TaskData& task = tasks[k];
		std::atomic<uint32_t>& finalStatus = reinterpret_cast<std::atomic<uint32_t>&>(task.status);
		if (task.status == TaskData::STATUS_BAKED) {
			finalStatus.store(TaskData::STATUS_POSTPROCESS);
			Cell& cell = *task.cell();
			task.next = cell.taskHead.load(std::memory_order_acquire);
			while (!cell.taskHead.compare_exchange_weak(task.next, &task, std::memory_order_release)) {}

			engine.GetKernel().GetThreadPool().Dispatch(CreateTaskContextFree(Wrap(this, &VisibilityComponent::PostProcess), task.cell), 1);
		} else if (task.status == TaskData::STATUS_ASSEMBLING) {
			if (task.pendingCount == 0) {
				// Commit draw calls.
				if (task.Continue()) {
					IRender::Queue* queue = task.renderQueue;
					IRender& render = engine.interfaces.render;

					for (size_t k = 0; k < task.dataUpdaters.size(); k++) {
						task.dataUpdaters[k]->Update(render, queue);
					}

					task.dataUpdaters.clear();

					IRender::Resource* buffer = render.CreateResource(render.GetQueueDevice(queue), IRender::Resource::RESOURCE_BUFFER);
					Bytes bufferData;
					uint32_t bufferSize = 0;
					std::vector<IRender::Resource*> drawCallResources;

					for (size_t i = 0; i < task.warpData.size(); i++) {
						TaskData::WarpData& warpData = task.warpData[i];
						std::unordered_map<size_t, InstanceGroup>& groups = warpData.instanceGroups;
						for (std::unordered_map<size_t, InstanceGroup>::iterator it = groups.begin(); it != groups.end(); ++it) {
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
									bufferRange.component = verify_cast<uint16_t>(viewSize / (group.instanceCount * sizeof(float)));
									warpData.bytesCache.Link(bufferData, data);
									bufferSize += verify_cast<uint32_t>(viewSize);
									assert(bufferData.GetViewSize() == bufferSize);
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

				finalStatus.store(TaskData::STATUS_ASSEMBLED, std::memory_order_release);
			}
		}
	}
}

void VisibilityComponent::CoTaskAssembleTask(Engine& engine, TaskData& task, const TShared<VisibilityComponent>& selfHolder) {
	OPTICK_EVENT();
	if (hostEntity == nullptr) return;

	IRender& render = engine.interfaces.render;
	assert(task.status == TaskData::STATUS_DISPATCHED);
	render.ExecuteResource(task.renderQueue, stateResource);
	render.ExecuteResource(task.renderQueue, task.renderTarget);

	const float t = 0.1f;
	const float f = 5000.0f;

	const float projectMatValues[] = {
		1.0f, 0, 0, 0,
		0, 1.0f, 0, 0,
		0.0f, 0.0f, (f + t) / (f - t), -1,
		0, 0, 2 * f * t / (f - t), 0
	};
	const double PI = 3.14159265358979323846;

	const MatrixFloat4x4 projectionMatrix(projectMatValues);
	PerspectiveCamera camera;
	camera.aspect = 1.0f;
	camera.farPlane = 5000.0f;
	camera.nearPlane = 0.1f;
	camera.fov = (float)PI / 2;
	task.camera = camera;

	const float r2 = (float)sqrt(2.0f) / 2.0f;
	static const Float3 directions[Cell::FACE_COUNT] = {
		Float3(r2, r2, 0),
		Float3(-r2, r2, 0),
		Float3(-r2, -r2, 0),
		Float3(r2, -r2, 0),
		Float3(0, 0, 1),
		Float3(0, 0, -1)
	};

	static const Float3 ups[Cell::FACE_COUNT] = {
		Float3(0, 0, 1),
		Float3(0, 0, 1),
		Float3(0, 0, 1),
		Float3(0, 0, 1),
		Float3(1, 0, 0),
		Float3(1, 0, 0),
	};

	const Int3& intPosition = task.cell->intPosition;
	Float3 viewPosition((intPosition.x() + 0.5f) * gridSize.x(), (intPosition.y() + 0.5f) * gridSize.y(), (intPosition.z() + 0.5f) * gridSize.z());

	for (size_t i = 0; i < task.warpData.size(); i++) {
		task.warpData[i].bytesCache.Reset();
	}

	CaptureData captureData;
	MatrixFloat4x4 viewMatrix = Math::MatrixLookAt(viewPosition, directions[task.faceIndex], ups[task.faceIndex]);
	MatrixFloat4x4 viewProjectionMatrix = viewMatrix * projectionMatrix;

	camera.UpdateCaptureData(captureData, Math::QuickInverse(viewMatrix));

	WorldInstanceData instanceData;
	instanceData.worldMatrix = viewProjectionMatrix;
	CollectComponentsFromEntity(engine, task, instanceData, captureData, hostEntity);

	std::atomic<uint32_t>& finalStatus = reinterpret_cast<std::atomic<uint32_t>&>(task.status);
	finalStatus.store(TaskData::STATUS_ASSEMBLING, std::memory_order_release);
}

void VisibilityComponent::DispatchTasks(Engine& engine) {
	OPTICK_EVENT();
	Kernel& kernel = engine.GetKernel();
	ThreadPool& threadPool = kernel.GetThreadPool();

	for (size_t i = 0, n = 0; i < bakePoints.size() && n < tasks.size(); i++) {
		const TShared<Cell>& cell = bakePoints[i];
		Entity* entity = hostEntity;

		for (uint32_t k = 0; k < Cell::FACE_COUNT; k++) {
			if (!(cell->dispatched.load(std::memory_order_relaxed) & (1 << k))) {
				while (n < tasks.size()) {
					TaskData& task = tasks[n++];

					if (task.status == TaskData::STATUS_START) {
						task.cell = cell;
						task.faceIndex = k;
						task.pendingResourceCount = 0;
						cell->dispatched.fetch_or(1 << k, std::memory_order_relaxed);

						std::atomic<uint32_t>& finalStatus = reinterpret_cast<std::atomic<uint32_t>&>(task.status);
						finalStatus.store(TaskData::STATUS_DISPATCHED, std::memory_order_release);
						threadPool.Dispatch(CreateCoTaskContextFree(kernel, Wrap(this, &VisibilityComponent::CoTaskAssembleTask), std::ref(engine), std::ref(task), this));
						break;
					}
				}

				if (n == tasks.size())
					break; // not enough slots
			}
		}
	}

	bakePoints.clear();
}

void VisibilityComponent::RoutineTickTasks(Engine& engine) {
	OPTICK_EVENT();

	// Must complete all pending resources
	if (engine.snowyStream.GetRenderResourceManager()->GetCompleted()) {
		ResolveTasks(engine);
		DispatchTasks(engine);
	}
}
