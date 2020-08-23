#include "VisibilityComponent.h"
#include "../../../SnowyStream/SnowyStream.h"
#include "../Space/SpaceComponent.h"
#include "../Renderable/RenderableComponent.h"
#include "../Transform/TransformComponent.h"
#include "../Explorer/ExplorerComponent.h"
#include "../../../../Core/Interface/IMemory.h"

using namespace PaintsNow;

VisibilityComponent::VisibilityComponent() : nextCoord(~(uint16_t)0, ~(uint16_t)0, ~(uint16_t)0), subDivision(1, 1, 1), taskCount(32), resolution(256, 256), maxFrameExecutionTime(5), viewDistance(512.0f), activeCellCacheIndex(0), hostEntity(nullptr), renderQueue(nullptr), clearResource(nullptr), depthStencilResource(nullptr), stateResource(nullptr) {
	maxVisIdentity.store(0, std::memory_order_relaxed);
	collectCritical.store(0, std::memory_order_relaxed);
}

Tiny::FLAG VisibilityComponent::GetEntityFlagMask() const {
	return Entity::ENTITY_HAS_TICK_EVENT | Entity::ENTITY_HAS_SPECIAL_EVENT;
}

TObject<IReflect>& VisibilityComponent::operator () (IReflect& reflect) {
	BaseClass::operator () (reflect);

	if (reflect.IsReflectProperty()) {
		ReflectProperty(taskCount);
		ReflectProperty(resolution);

		ReflectProperty(boundingBox);
		ReflectProperty(subDivision);
		ReflectProperty(maxFrameExecutionTime);
		ReflectProperty(viewDistance);
	}

	return *this;
}

void VisibilityComponent::Initialize(Engine& engine, Entity* entity) {
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

	IRender& render = engine.interfaces.render;
	IRender::Device* device = engine.snowyStream.GetRenderDevice();
	renderQueue = render.CreateQueue(device);

	String path = ShaderResource::GetShaderPathPrefix() + UniqueType<ConstMapPass>::Get()->GetBriefName();
	pipeline = engine.snowyStream.CreateReflectedResource(UniqueType<ShaderResource>(), path, true, 0, nullptr)->QueryInterface(UniqueType<ShaderResourceImpl<ConstMapPass> >());

	IRender::Resource::ClearDescription cls;
	cls.clearColorBit = IRender::Resource::ClearDescription::CLEAR;
	cls.clearDepthBit = IRender::Resource::ClearDescription::CLEAR;
	cls.clearStencilBit = IRender::Resource::ClearDescription::DISCARD_LOAD | IRender::Resource::ClearDescription::DISCARD_STORE;

	clearResource = render.CreateResource(device, IRender::Resource::RESOURCE_CLEAR);
	render.UploadResource(renderQueue, clearResource, &cls);

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
		texture = engine.snowyStream.CreateReflectedResource(UniqueType<TextureResource>(), ResourceBase::GenerateLocation("VisBake", &task), false, 0, nullptr);
		texture->description.dimension = dim;
		texture->description.state.attachment = true;
		texture->description.state.format = IRender::Resource::TextureDescription::UNSIGNED_BYTE;
		texture->description.state.layout = IRender::Resource::TextureDescription::RGBA;
		texture->Flag().fetch_or(Tiny::TINY_MODIFIED, std::memory_order_release);
		texture->GetResourceManager().InvokeUpload(texture(), renderQueue);

		IRender::Resource::RenderTargetDescription desc;
		desc.colorBufferStorages.resize(1);
		IRender::Resource::RenderTargetDescription::Storage& s = desc.colorBufferStorages[0];
		s.resource = texture->GetTexture();
		desc.depthStencilStorage.resource = depthStencilResource;
		task.renderTarget = render.CreateResource(device, IRender::Resource::RESOURCE_RENDERTARGET);
		render.UploadResource(renderQueue, task.renderTarget, &desc);
	}

	BaseComponent::Initialize(engine, entity);
}

void VisibilityComponent::Uninitialize(Engine& engine, Entity* entity) {
	assert(hostEntity != nullptr);
	hostEntity = nullptr;
	IRender& render = engine.interfaces.render;
	for (size_t k = 0; k < tasks.size(); k++) {
		TaskData& task = tasks[k];
		render.DeleteResource(task.renderQueue, task.renderTarget);
		render.DeleteQueue(task.renderQueue);
	}

	tasks.clear();

	render.DeleteResource(renderQueue, clearResource);
	render.DeleteResource(renderQueue, stateResource);
	render.DeleteResource(renderQueue, depthStencilResource);
	render.DeleteQueue(renderQueue);

	clearResource = stateResource = depthStencilResource = nullptr;
	renderQueue = nullptr;
	BaseComponent::Uninitialize(engine, entity);
}

Entity* VisibilityComponent::GetHostEntity() const {
	return hostEntity;
}

void VisibilityComponent::DispatchEvent(Event& event, Entity* entity) {
	if (event.eventID == Event::EVENT_FRAME) {
		// DoBake
		TickRender(event.engine);

		// Prepare
		Engine& engine = event.engine;
		engine.GetKernel().QueueRoutine(this, CreateTaskContextFree(Wrap(this, &VisibilityComponent::RoutineTickTasks), std::ref(engine)));
	}
}

void VisibilityComponent::Setup(Engine& engine, float distance, const Float3Pair& range, const UShort3& division, uint32_t frameTimeLimit, uint32_t tc, const UShort2& resolution) {
	assert(!(Flag() & TINY_MODIFIED));
	Flag().fetch_or(TINY_MODIFIED, std::memory_order_acquire);

	taskCount = tc;
	boundingBox = range;
	subDivision = division;
	maxFrameExecutionTime = frameTimeLimit;
	viewDistance = distance;
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

const Bytes& VisibilityComponent::QuerySample(const Float3& position) {
	Float3 offset = (position - boundingBox.first) / (boundingBox.second - boundingBox.first);

	// out of range
	if (offset.x() < 0 || offset.y() < 0 || offset.z() < 0 || offset.x() > 1 || offset.y() > 1 || offset.z() > 1) return Bytes::Null();

	UShort3 coord;
	for (uint32_t k = 0; k < 3; k++) {
		coord[k] = (uint16_t)Math::Min((int)floor((subDivision[k] - 1) * offset[k] + 0.5), subDivision[k] - 1);
	}

	// load cache
	Cache* cache = nullptr;
	for (uint32_t i = 0; i < sizeof(cellCache) / sizeof(cellCache[0]); i++) {
		Cache& current = cellCache[i];
		if (current.counter != 0 && current.index == coord) {
			// hit!
			return current.mergedPayload;
		}
	}

	// cache miss
	UShort3 lowerBound((uint16_t)Math::Max(coord.x() - 1, 0), (uint16_t)Math::Max(coord.y() - 1, 0), (uint16_t)Math::Max(coord.z() - 1, 0));
	UShort3 upperBound((uint16_t)Math::Min(coord.x() + 1, (int)subDivision.x() - 1), (uint16_t)Math::Min(coord.y() + 1, (int)subDivision.y() - 1), (uint16_t)Math::Min(coord.z() + 1, (int)subDivision.z() - 1));

	Cell& center = cells[coord];
	if (center.incompleteness != 0) {
		if (nextCoord != coord) {
			nextCoord = coord;
			Flag().fetch_and(~VISIBILITYCOMPONENT_NEXT_PROCESSING, std::memory_order_release);
		}

		return Bytes::Null();
	} else {
		std::vector<Bytes*> toMerge;
		toMerge.reserve(9);
		uint32_t mergedSize = 0;

		for (uint16_t z = lowerBound.z(); z <= upperBound.z(); z++) {
			for (uint16_t y = lowerBound.y(); y <= upperBound.y(); y++) {
				for (uint16_t x = lowerBound.x(); x <= upperBound.x(); x++) {
					UShort3 coord(x, y, z);
					Cell& cell = cells[coord];
					if (!cell.payload.Empty()) {
						mergedSize = Math::Max(mergedSize, cell.payload.GetSize());
						toMerge.emplace_back(&cell.payload);
					}
				}
			}
		}

		// Do Merge
		Bytes mergedPayload;
		mergedPayload.Resize(mergedSize, 0);
		for (size_t i = 0; i < toMerge.size(); i++) {
			MergeSample(mergedPayload, *toMerge[i]);
		}

		Cache& targetCache = cellCache[activeCellCacheIndex];
		targetCache.index = coord;
		targetCache.counter = 1;
		targetCache.mergedPayload = std::move(mergedPayload);
		activeCellCacheIndex = (activeCellCacheIndex + 1) % (sizeof(cellCache) / sizeof(cellCache[0]));
		return targetCache.mergedPayload;
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

// Cell

VisibilityComponent::Cell::Cell() : incompleteness(27 * 6) {}

// Baker

void VisibilityComponent::TickRender(Engine& engine) {
	// check pipeline state
	if (renderQueue == nullptr) return; // not inited.
	IRender& render = engine.interfaces.render;
	render.PresentQueues(&renderQueue, 1, IRender::PRESENT_EXECUTE_ALL);

	// read previous results back and collect ready ones
	std::vector<IRender::Queue*> bakeQueues;
	for (size_t i = 0; i < tasks.size(); i++) {
		TaskData& task = tasks[i];
		TextureResource* texture = task.texture();
		std::atomic<uint32_t>& finalStatus = reinterpret_cast<std::atomic<uint32_t>&>(task.status);
		if (task.status == TaskData::STATUS_IDLE) {
			finalStatus.store(TaskData::STATUS_START, std::memory_order_release);
		} else if (task.status == TaskData::STATUS_ASSEMBLED) {
			render.RequestDownloadResource(task.renderQueue, texture->GetTexture(), &texture->description);
			render.FlushQueue(task.renderQueue);
			bakeQueues.emplace_back(task.renderQueue);
			finalStatus.store(TaskData::STATUS_BAKING, std::memory_order_release);
		} else if (task.status == TaskData::STATUS_BAKING) {
			render.CompleteDownloadResource(task.renderQueue, texture->GetTexture());
			bakeQueues.emplace_back(task.renderQueue);
			task.data = std::move(texture->description.data);
			finalStatus.store(TaskData::STATUS_BAKED, std::memory_order_release);
		}
	}

	// Commit bakes
	if (!bakeQueues.empty()) {
		render.PresentQueues(&bakeQueues[0], safe_cast<uint32_t>(bakeQueues.size()), IRender::PRESENT_EXECUTE_ALL);
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
	std::map<size_t, InstanceGroup>& instanceGroups = task.instanceGroups[currentWarpIndex == ~(uint32_t)0 ? GetWarpIndex() : currentWarpIndex];
	InstanceGroup& first = instanceGroups[(size_t)renderableComponent];

	std::vector<IRender::Resource*> textureResources;
	std::vector<IRender::Resource::DrawCallDescription::BufferRange> bufferResources;

	if (first.drawCallDescription.shaderResource != nullptr) {
		size_t i = 0;
		while (true) {
			InstanceGroup& group = instanceGroups[(size_t)renderableComponent + i++];
			if (group.drawCallDescription.shaderResource == nullptr) break;
	
			std::vector<Bytes> s;
			group.instanceUpdater.Snapshot(s, bufferResources, textureResources, instanceData);
			assert(s.size() == group.instancedData.size());
			for (size_t k = 0; k < s.size(); k++) {
				group.instancedData[k].Append(s[k]);
			}

			group.instanceCount++;
		}
	} else {
		IDrawCallProvider::InputRenderData inputRenderData(0.0f, pipeline());
		std::vector<IDrawCallProvider::OutputRenderData> drawCalls;
		SpinLock(collectCritical);
		renderableComponent->CollectDrawCalls(drawCalls, inputRenderData);
		SpinUnLock(collectCritical);
		assert(drawCalls.size() < sizeof(RenderableComponent) - 1);

		for (size_t i = 0; i < drawCalls.size(); i++) {
			IDataUpdater* dataUpdater = drawCalls[i].dataUpdater;
			InstanceGroup& group = instanceGroups[(size_t)renderableComponent + i];
			if (group.instanceCount == 0) {
				std::binary_insert(task.dataUpdaters, dataUpdater);

				instanceData.Export(group.instanceUpdater, pipeline->GetPassUpdater());
				group.drawCallDescription = std::move(drawCalls[i].drawCallDescription);
				group.instanceUpdater.Snapshot(group.instancedData, bufferResources, textureResources, instanceData);
			} else {
				std::vector<Bytes> s;
				group.instanceUpdater.Snapshot(s, bufferResources, textureResources, instanceData);
				assert(s.size() == group.instancedData.size());
				for (size_t k = 0; k < s.size(); k++) {
					group.instancedData[k].Append(s[k]);
				}
			}

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

		if (component->GetEntityFlagMask() & Entity::ENTITY_HAS_RENDERABLE) {
			if (transformComponent != nullptr) {
				RenderableComponent* renderableComponent = static_cast<RenderableComponent*>(component);
				CollectRenderableComponent(engine, task, renderableComponent, instanceData, transformComponent->GetObjectID());
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

void VisibilityComponent::ResolveTasks(Engine& engine) {
	// resolve finished tasks
	for (size_t k = 0; k < tasks.size(); k++) {
		TaskData& task = tasks[k];
		std::atomic<uint32_t>& finalStatus = reinterpret_cast<std::atomic<uint32_t>&>(task.status);
		if (task.status == TaskData::STATUS_BAKED) {
			UShort3 coord = task.coord;
			Cell& cell = cells[coord];
			Bytes& encodedData = cell.payload;
			const Bytes& data = task.data;
			assert(data.GetSize() % sizeof(uint32_t) == 0);
			uint32_t count = data.GetSize() / sizeof(uint32_t);
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

			// scan neighbors
			UShort3 lowerBound((uint16_t)Math::Max(coord.x() - 1, 0), (uint16_t)Math::Max(coord.y() - 1, 0), (uint16_t)Math::Max(coord.z() - 1, 0));
			UShort3 upperBound((uint16_t)Math::Min(coord.x() + 1, (int)subDivision.x() - 1), (uint16_t)Math::Min(coord.y() + 1, (int)subDivision.y() - 1), (uint16_t)Math::Min(coord.z() + 1, (int)subDivision.z() - 1));

			for (uint16_t z = lowerBound.z(); z <= upperBound.z(); z++) {
				for (uint16_t y = lowerBound.y(); y <= upperBound.y(); y++) {
					for (uint16_t x = lowerBound.x(); x <= upperBound.x(); x++) {
						UShort3 t(x, y, z);
						Cell& c = cells[t];
						assert(c.incompleteness != 0);
						--c.incompleteness;
					}
				}
			}

			finalStatus.store(TaskData::STATUS_IDLE);
			// printf("Bake task (%d, %d, %d) [Remains %d] .\n", coord.x(), coord.y(), coord.z(), cell.incompleteness);
		} else if (task.status == TaskData::STATUS_ASSEMBLING) {
			if (task.pendingCount == 0) {
				// Commit draw calls.
				IRender::Queue* queue = task.renderQueue;
				IRender& render = engine.interfaces.render;

				for (size_t k = 0; k < task.dataUpdaters.size(); k++) {
					task.dataUpdaters[k]->Update(render, queue);
				}

				task.dataUpdaters.clear();

				IRender::Resource* buffer = render.CreateResource(render.GetQueueDevice(queue), IRender::Resource::RESOURCE_BUFFER);
				Bytes bufferData;
				std::vector<IRender::Resource*> drawCallResources;

				for (size_t i = 0; i < task.instanceGroups.size(); i++) {
					std::map<size_t, InstanceGroup>& groups = task.instanceGroups[i];
					for (std::map<size_t, InstanceGroup>::iterator it = groups.begin(); it != groups.end(); ++it) {
						InstanceGroup& group = it->second;
						if (group.drawCallDescription.shaderResource == nullptr || group.instanceCount == 0) continue;

						for (size_t k = 0; k < group.instancedData.size(); k++) {
							Bytes& data = group.instancedData[k];
							assert(!data.Empty());
							if (!data.Empty()) {
								PassBase::Parameter& output = group.instanceUpdater.parameters[k];
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
						PassBase::ValidateDrawCall(group.drawCallDescription);

						IRender::Resource* drawCall = render.CreateResource(render.GetQueueDevice(queue), IRender::Resource::RESOURCE_DRAWCALL);
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
				finalStatus.store(TaskData::STATUS_ASSEMBLED, std::memory_order_release);
			}
		}
	}
}

void VisibilityComponent::CoTaskAssembleTask(Engine& engine, TaskData& task, const BakePoint& bakePoint) {
	if (hostEntity == nullptr) return;
	IRender& render = engine.interfaces.render;
	assert(task.status == TaskData::STATUS_DISPATCHED);
	render.ExecuteResource(task.renderQueue, task.renderTarget);
	render.ExecuteResource(task.renderQueue, stateResource);
	render.ExecuteResource(task.renderQueue, clearResource);

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
	static const Float3 directions[6] = {
		Float3(r2, r2, 0),
		Float3(-r2, r2, 0),
		Float3(-r2, -r2, 0),
		Float3(r2, -r2, 0),
		Float3(0, 0, 1),
		Float3(0, 0, -1)
	};

	static const Float3 ups[6] = {
		Float3(0, 0, 1),
		Float3(0, 0, 1),
		Float3(0, 0, 1),
		Float3(0, 0, 1),
		Float3(1, 0, 0),
		Float3(1, 0, 0),
	};


	uint16_t i = bakePoint.coord.x();
	uint16_t j = bakePoint.coord.y();
	uint16_t k = bakePoint.coord.z();

	task.coord = bakePoint.coord;
	uint16_t face = bakePoint.face;
	Float3 viewPosition = boundingBox.first + (boundingBox.second - boundingBox.first) * Float3((float)(i + 0.5f) / subDivision.x(), (float)(j + 0.5f) / subDivision.y(), (float)(k + 0.5f) / subDivision.z());
	task.instanceGroups.resize(engine.GetKernel().GetWarpCount());
	CaptureData captureData;
	MatrixFloat4x4 viewMatrix = Math::LookAt(viewPosition, directions[face], ups[face]);
	MatrixFloat4x4 viewProjectionMatrix = viewMatrix * projectionMatrix;

	camera.UpdateCaptureData(captureData, Math::QuickInverse(viewMatrix));

	WorldInstanceData instanceData;
	instanceData.worldMatrix = viewProjectionMatrix;

	CollectComponentsFromEntity(engine, task, instanceData, captureData, hostEntity);

	std::atomic<uint32_t>& finalStatus = reinterpret_cast<std::atomic<uint32_t>&>(task.status);
	finalStatus.store(TaskData::STATUS_ASSEMBLING, std::memory_order_release);
}

void VisibilityComponent::DispatchTasks(Engine& engine) {
	size_t n = 0;
	Kernel& kernel = engine.GetKernel();
	ThreadPool& threadPool = kernel.threadPool;

	while (true) {
		while (!bakePoints.empty()) {
			BakePoint bakePoint = bakePoints.top();
			Cell& cell = cells[bakePoint.coord];
			// already baked?
			if (cell.incompleteness != 0) {
				Entity* entity = hostEntity;
				// find idle task
				while (n < tasks.size()) {
					TaskData& task = tasks[n];
					std::atomic<uint32_t>& finalStatus = reinterpret_cast<std::atomic<uint32_t>&>(task.status);
					if (task.status == TaskData::STATUS_START) {
						finalStatus.store(TaskData::STATUS_DISPATCHED, std::memory_order_release);
						threadPool.Push(CreateCoTaskContextFree(kernel, Wrap(this, &VisibilityComponent::CoTaskAssembleTask), std::ref(engine), std::ref(task), bakePoint));
						break;
					}

					n++;
				}

				// no more slots ... go next frame
				if (n == tasks.size())
					break;
			}

			bakePoints.pop();
		}

		if (!bakePoints.empty()) {
			break;
		} else {
			if (Flag() & VISIBILITYCOMPONENT_NEXT_PROCESSING)
				break;

			UShort3 coord = nextCoord;
			if (coord.x() == ~(uint16_t)0)
				break;

			Cell& center = cells[coord];
			if (center.incompleteness == 0)
				break;

			UShort3 lowerBound((uint16_t)Math::Max(coord.x() - 1, 0), (uint16_t)Math::Max(coord.y() - 1, 0), (uint16_t)Math::Max(coord.z() - 1, 0));
			UShort3 upperBound((uint16_t)Math::Min(coord.x() + 1, (int)subDivision.x() - 1), (uint16_t)Math::Min(coord.y() + 1, (int)subDivision.y() - 1), (uint16_t)Math::Min(coord.z() + 1, (int)subDivision.z() - 1));

			for (uint16_t z = lowerBound.z(); z <= upperBound.z(); z++) {
				for (uint16_t y = lowerBound.y(); y <= upperBound.y(); y++) {
					for (uint16_t x = lowerBound.x(); x <= upperBound.x(); x++) {
						UShort3 coord(x, y, z);
						Cell& cell = cells[coord];
						if (cell.payload.Empty()) {
							// Add new tasks
							BakePoint bakePoint;
							bakePoint.coord = coord;
							cell.payload.Resize(sizeof(uint32_t), 0);
							for (uint16_t f = 0; f < 6; f++) {
								bakePoint.face = f;
								bakePoints.push(bakePoint);
							}
						}
					}
				}
			}

			Flag().fetch_or(VISIBILITYCOMPONENT_NEXT_PROCESSING, std::memory_order_release);
		}
	}
}

void VisibilityComponent::RoutineTickTasks(Engine& engine) {
	ResolveTasks(engine);
	DispatchTasks(engine);
}
