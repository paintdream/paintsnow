#include "PhaseComponent.h"
#include "../Light/LightComponent.h"
#include "../Space/SpaceComponent.h"
#include "../Renderable/RenderableComponent.h"
#include "../Transform/TransformComponent.h"
#include "../Explorer/ExplorerComponent.h"
#include <cmath>
#include <ctime>
#include <utility>

const float PI = 3.1415926f;
static inline float RandFloat() {
	return (float)rand() / RAND_MAX;
}

using namespace PaintsNow;

PhaseComponent::LightConfig::TaskData::TaskData(uint32_t warpCount) : pendingCount(0) {
	warpData.resize(warpCount);
}

TObject<IReflect>& PhaseComponentConfig::WorldGlobalData::operator () (IReflect& reflect) {
	BaseClass::operator () (reflect);

	if (reflect.IsReflectProperty()) {
		ReflectProperty(viewProjectionMatrix)[IShader::BindInput(IShader::BindInput::TRANSFORM_VIEWPROJECTION)];
		ReflectProperty(viewMatrix)[IShader::BindInput(IShader::BindInput::TRANSFORM_VIEW)];
		ReflectProperty(noiseTexture);
	}

	return *this;
}

TObject<IReflect>& PhaseComponentConfig::WorldInstanceData::operator () (IReflect& reflect) {
	BaseClass::operator () (reflect);

	if (reflect.IsReflectProperty()) {
		ReflectProperty(worldMatrix)[IShader::BindInput(IShader::BindInput::TRANSFORM_WORLD)];
		ReflectProperty(instancedColor)[IShader::BindInput(IShader::BindInput::COLOR_INSTANCED)];
	}

	return *this;
}

PhaseComponentConfig::TaskData::TaskData() : status(STATUS_IDLE), pendingCount(0), renderQueue(nullptr), renderTarget(nullptr), pipeline(nullptr) {
}

PhaseComponentConfig::TaskData::~TaskData() {}

void PhaseComponentConfig::InstanceGroup::Reset() {
	for (size_t k = 0; k < instancedData.size(); k++) {
		instancedData[k].Clear();
	}

	instanceCount = 0;
}

PhaseComponent::PhaseComponent(const TShared<RenderFlowComponent>& renderFlow, const String& portName) : hostEntity(nullptr), maxTracePerTick(8), renderQueue(nullptr), stateResource(nullptr), stateShadowResource(nullptr), range(32, 32, 32), resolution(512, 512), lightCollector(this), renderFlowComponent(std::move(renderFlow)), lightPhaseViewPortName(portName), rootEntity(nullptr) {}

PhaseComponent::~PhaseComponent() {}

void PhaseComponent::Initialize(Engine& engine, Entity* entity) {
	if (rootEntity != entity) { // Set Host?
		Component::Initialize(engine, entity);
		assert(hostEntity == nullptr);
		assert(renderQueue == nullptr);
		hostEntity = entity;

		String location = ResourceBase::GenerateLocation("PhaseEmptyColorAttachment", (void*)(((size_t)resolution.x() << 16) | resolution.y()));
		emptyColorAttachment = engine.snowyStream.CreateReflectedResource(UniqueType<TextureResource>(), location, false, ResourceBase::RESOURCE_VIRTUAL);

		if (!emptyColorAttachment) {
			emptyColorAttachment = engine.snowyStream.CreateReflectedResource(UniqueType<TextureResource>(), location, false, ResourceBase::RESOURCE_VIRTUAL);
			emptyColorAttachment->description.state.attachment = true;
			emptyColorAttachment->description.dimension = UShort3(resolution.x(), resolution.y(), 1);
			emptyColorAttachment->description.state.format = IRender::Resource::TextureDescription::UNSIGNED_BYTE;
			emptyColorAttachment->description.state.layout = IRender::Resource::TextureDescription::R;
			emptyColorAttachment->Flag().fetch_or(Tiny::TINY_MODIFIED, std::memory_order_release);

			IRender::Queue* queue = engine.snowyStream.GetResourceQueue();
			emptyColorAttachment->GetResourceManager().InvokeUpload(emptyColorAttachment(), queue);
		}

		SnowyStream& snowyStream = engine.snowyStream;
		const String path = "[Runtime]/MeshResource/StandardQuad";
		meshResource = snowyStream.CreateReflectedResource(UniqueType<MeshResource>(), path, true, ResourceBase::RESOURCE_VIRTUAL);

		tracePipeline = snowyStream.CreateReflectedResource(UniqueType<ShaderResource>(), ShaderResource::GetShaderPathPrefix() + UniqueType<MultiHashTracePass>::Get()->GetBriefName(), true, ResourceBase::RESOURCE_VIRTUAL)->QueryInterface(UniqueType<ShaderResourceImpl<MultiHashTracePass> >());
		setupPipeline = snowyStream.CreateReflectedResource(UniqueType<ShaderResource>(), ShaderResource::GetShaderPathPrefix() + UniqueType<MultiHashSetupPass>::Get()->GetBriefName(), true, ResourceBase::RESOURCE_VIRTUAL)->QueryInterface(UniqueType<ShaderResourceImpl<MultiHashSetupPass> >());
		shadowPipeline = snowyStream.CreateReflectedResource(UniqueType<ShaderResource>(), ShaderResource::GetShaderPathPrefix() + UniqueType<ConstMapPass>::Get()->GetBriefName(), true, ResourceBase::RESOURCE_VIRTUAL)->QueryInterface(UniqueType<ShaderResourceImpl<ConstMapPass> >());

		IRender& render = engine.interfaces.render;
		IRender::Device* device = engine.snowyStream.GetRenderDevice();
		renderQueue = render.CreateQueue(device);

		stateResource = render.CreateResource(device, IRender::Resource::RESOURCE_RENDERSTATE);
		IRender::Resource::RenderStateDescription state;
		state.cull = 1;
		state.fill = 1;
		state.blend = 0;
		state.colorWrite = 1;
		state.depthTest = IRender::Resource::RenderStateDescription::DISABLED;
		state.depthWrite = 0;
		state.stencilTest = IRender::Resource::RenderStateDescription::DISABLED;
		state.stencilWrite = 0;
		render.UploadResource(renderQueue, stateResource, &state);

		state.cull = 1;
		state.fill = 1;
		state.blend = 0;
		state.colorWrite = 0;
		state.depthTest = IRender::Resource::RenderStateDescription::GREATER_EQUAL;
		state.depthWrite = 1;
		state.stencilTest = IRender::Resource::RenderStateDescription::DISABLED;
		state.stencilWrite = 0;
		state.cullFrontFace = 1;
		stateShadowResource = render.CreateResource(device, IRender::Resource::RESOURCE_RENDERSTATE);
		render.UploadResource(renderQueue, stateShadowResource, &state);
	}
}

Tiny::FLAG PhaseComponent::GetEntityFlagMask() const {
	return Entity::ENTITY_HAS_TICK_EVENT | Entity::ENTITY_HAS_SPECIAL_EVENT;
}

void PhaseComponent::Uninitialize(Engine& engine, Entity* entity) {
	if (rootEntity == entity) {
		rootEntity = nullptr;
	} else {
		IRender::Queue* queue = reinterpret_cast<IRender::Queue*>(tracePipeline->GetResourceManager().GetContext());
		IRender& render = engine.interfaces.render;
		for (size_t j = 0; j < tasks.size(); j++) {
			TaskData& task = tasks[j];
			render.DeleteResource(task.renderQueue, task.renderTarget);
			render.DeleteQueue(task.renderQueue);
		}

		tasks.clear();

		for (size_t k = 0; k < phases.size(); k++) {
			Phase& phase = phases[k];
			for (size_t j = 0; j < phase.uniformBuffers.size(); j++) {
				render.DeleteResource(queue, phase.uniformBuffers[j]);
			}

			render.DeleteResource(queue, phase.drawCallResource);
		}

		phases.clear();

		render.DeleteResource(queue, stateResource);
		render.DeleteResource(queue, stateShadowResource);
		render.DeleteQueue(renderQueue);

		stateResource = stateShadowResource = nullptr;
		renderQueue = nullptr;
		hostEntity = nullptr;

		Component::Uninitialize(engine, entity);
	}
}

void PhaseComponent::Resample(Engine& engine, uint32_t phaseCount) {
	// TODO: resample phases.
}

void PhaseComponent::Step(Engine& engine, uint32_t bounceCount) {
	// generate pairs of phases
	assert(phases.size() >= 2);
	for (uint32_t i = 0; i < bounceCount; i++) {
		Phase* from = &phases[rand() % phases.size()];
		Phase* to;
		do {
			to = &phases[rand() % phases.size()];
		} while (to == from);

		UpdatePointBounce bounce;
		bounce.fromPhaseIndex = safe_cast<uint32_t>(from - &phases[0]);
		bounce.toPhaseIndex = safe_cast<uint32_t>(to - &phases[0]);

		bakePointBounces.push(bounce);
	}
}

void PhaseComponent::Setup(Engine& engine, uint32_t phaseCount, uint32_t taskCount, const Float3& r, const UShort2& res) {
	assert(tasks.empty() && phases.empty());
	IRender& render = engine.interfaces.render;
	IRender::Device* device = engine.snowyStream.GetRenderDevice();
	SnowyStream& snowyStream = engine.snowyStream;
	resolution = res;
	range = r;

	uint32_t warpCount = engine.GetKernel().GetWarpCount();
	tasks.resize(taskCount);
	for (size_t j = 0; j < tasks.size(); j++) {
		TaskData& task = tasks[j];
		task.warpData.resize(warpCount);
		task.renderQueue = render.CreateQueue(device);
		task.renderTarget = render.CreateResource(device, IRender::Resource::RESOURCE_RENDERTARGET);
	}

	// prepare uniform buffers for tracing
	phases.resize(phaseCount);
	for (size_t i = 0; i < phases.size(); i++) {
		Phase& phase = phases[i];

		std::vector<Bytes> bufferData;
		phase.tracePipeline.Reset(static_cast<ShaderResourceImpl<MultiHashTracePass>*>(tracePipeline->Clone()));
		phase.tracePipeline->GetPassUpdater().Capture(phase.drawCallDescription, bufferData, 1 << IRender::Resource::BufferDescription::UNIFORM);
		phase.tracePipeline->GetPassUpdater().Update(render, renderQueue, phase.drawCallDescription, phase.uniformBuffers, bufferData, 1 << IRender::Resource::BufferDescription::UNIFORM);
		phase.drawCallResource = render.CreateResource(device, IRender::Resource::RESOURCE_DRAWCALL);

		phase.depth = engine.snowyStream.CreateReflectedResource(UniqueType<TextureResource>(), ResourceBase::GenerateLocation("PhaseDepth", &phase), false, ResourceBase::RESOURCE_VIRTUAL);

		phase.depth->description.state.attachment = true;
		phase.depth->description.dimension = UShort3(resolution.x(), resolution.y(), 1);
		phase.depth->description.state.format = IRender::Resource::TextureDescription::FLOAT;
		phase.depth->description.state.layout = IRender::Resource::TextureDescription::DEPTH;
		phase.depth->Flag().fetch_or(Tiny::TINY_MODIFIED, std::memory_order_release);
		phase.depth->GetResourceManager().InvokeUpload(phase.depth(), renderQueue);

		phase.irradiance = engine.snowyStream.CreateReflectedResource(UniqueType<TextureResource>(), ResourceBase::GenerateLocation("PhaseIrradiance", &phase), false, ResourceBase::RESOURCE_VIRTUAL);
		phase.irradiance->description.state.attachment = true;
		phase.irradiance->description.dimension = UShort3(resolution.x(), resolution.y(), 1);
		phase.irradiance->description.state.format = IRender::Resource::TextureDescription::HALF;
		phase.irradiance->description.state.layout = IRender::Resource::TextureDescription::RGBA;
		phase.irradiance->Flag().fetch_or(Tiny::TINY_MODIFIED, std::memory_order_release);
		phase.irradiance->GetResourceManager().InvokeUpload(phase.irradiance(), renderQueue);

		phase.baseColorOcclusion = engine.snowyStream.CreateReflectedResource(UniqueType<TextureResource>(), ResourceBase::GenerateLocation("PhaseBaseColorOcclusion", &phase), false, ResourceBase::RESOURCE_VIRTUAL);
		phase.baseColorOcclusion->description.state.attachment = true;
		phase.baseColorOcclusion->description.dimension = UShort3(resolution.x(), resolution.y(), 1);
		phase.baseColorOcclusion->description.state.format = IRender::Resource::TextureDescription::UNSIGNED_BYTE;
		phase.baseColorOcclusion->description.state.layout = IRender::Resource::TextureDescription::RGBA;
		phase.baseColorOcclusion->Flag().fetch_or(Tiny::TINY_MODIFIED, std::memory_order_release);
		phase.baseColorOcclusion->GetResourceManager().InvokeUpload(phase.baseColorOcclusion(), renderQueue);

		phase.normalRoughnessMetallic = engine.snowyStream.CreateReflectedResource(UniqueType<TextureResource>(), ResourceBase::GenerateLocation("PhaseNormalRoughness", &phase), false, ResourceBase::RESOURCE_VIRTUAL);
		phase.normalRoughnessMetallic->description.state.attachment = true;
		phase.normalRoughnessMetallic->description.dimension = UShort3(resolution.x(), resolution.y(), 1);
		phase.normalRoughnessMetallic->description.state.format = IRender::Resource::TextureDescription::UNSIGNED_BYTE;
		phase.normalRoughnessMetallic->description.state.layout = IRender::Resource::TextureDescription::RGBA;
		phase.normalRoughnessMetallic->Flag().fetch_or(Tiny::TINY_MODIFIED, std::memory_order_relaxed);
		phase.normalRoughnessMetallic->GetResourceManager().InvokeUpload(phase.normalRoughnessMetallic(), renderQueue);

		// create noise texture
		phase.noiseTexture = engine.snowyStream.CreateReflectedResource(UniqueType<TextureResource>(), ResourceBase::GenerateLocation("PhaseNoise", &phase), false, ResourceBase::RESOURCE_VIRTUAL);
		phase.noiseTexture->description.dimension = UShort3(resolution.x(), resolution.y(), 1);
		phase.noiseTexture->description.state.format = IRender::Resource::TextureDescription::UNSIGNED_BYTE;
		phase.noiseTexture->description.state.layout = IRender::Resource::TextureDescription::RGBA;
		
		Bytes& bytes = phase.noiseTexture->description.data;
		bytes.Resize(resolution.x() * resolution.y() * 4);
#if RAND_MAX == 0x7FFFFFFF
		uint32_t* buffer = reinterpret_cast<uint32_t*>(bytes.GetData());
		for (size_t k = 0; k < resolution.x() * resolution.y() * 4 / sizeof(uint32_t); k++) {
			buffer[k] = rand() | (rand() & 1 ? 0x80000000 : 0);
		}
#elif RAND_MAX == 0x7FFF
		uint16_t* buffer = reinterpret_cast<uint16_t*>(bytes.GetData());
		for (size_t k = 0; k < resolution.x() * resolution.y() * 4 / sizeof(uint16_t); k++) {
			buffer[k] = rand() | (rand() & 1 ? 0x8000 : 0);
		}
#else
		static_assert(false, "Unrecognized RAND_MAX");
#endif
		phase.noiseTexture->Flag().fetch_or(Tiny::TINY_MODIFIED, std::memory_order_release);
		phase.noiseTexture->GetResourceManager().InvokeUpload(phase.noiseTexture(), renderQueue);
	}
}

void PhaseComponent::DispatchEvent(Event& event, Entity* entity) {
	if (entity != rootEntity) {
		if (event.eventID == Event::EVENT_FRAME) {
			// DoUpdate
			TickRender(event.engine);
		} else if (event.eventID == Event::EVENT_TICK) {
			Engine& engine = event.engine;
			ResolveTasks(engine);
			DispatchTasks(engine);
			UpdateRenderFlow(engine);
		}
	}
}

// #include "../../../MythForest/MythForest.h"

void PhaseComponent::TickRender(Engine& engine) {
	// check pipeline state
	if (renderQueue == nullptr) return; // not inited.

	IRender& render = engine.interfaces.render;
	Kernel& kernel = engine.GetKernel();
	render.PresentQueues(&renderQueue, 1, IRender::PRESENT_EXECUTE_ALL);

	std::vector<IRender::Queue*> bakeQueues;
	for (size_t i = 0; i < tasks.size(); i++) {
		TaskData& task = tasks[i];
		std::atomic<uint32_t>& finalStatus = reinterpret_cast<std::atomic<uint32_t>&>(task.status);
		if (task.status == TaskData::STATUS_IDLE) {
			finalStatus.store(TaskData::STATUS_START, std::memory_order_release);
		} else if (task.status == TaskData::STATUS_ASSEMBLED) {
			uint32_t status = TaskData::STATUS_BAKED;
			if (!debugPath.empty() && task.texture) {
				render.RequestDownloadResource(task.renderQueue, task.texture->GetRenderResource(), &task.texture->description);
				status = TaskData::STATUS_DOWNLOADED;
			}

			// engine.mythForest.StartCaptureFrame("cap", "");
			render.FlushQueue(task.renderQueue);
			bakeQueues.emplace_back(task.renderQueue);
			finalStatus.store(status, std::memory_order_release);
			// render.PresentQueues(&task.renderQueue, 1, IRender::PRESENT_EXECUTE_ALL);
			// engine.mythForest.EndCaptureFrame();
		} else if (task.status == TaskData::STATUS_DOWNLOADED) {
			render.CompleteDownloadResource(task.renderQueue, task.texture->GetRenderResource());
			bakeQueues.emplace_back(task.renderQueue);

			// Save data asynchronized
			uint32_t frameIndex = engine.GetFrameIndex();
			engine.GetKernel().QueueRoutine(this, CreateTaskContextFree(Wrap(this, &PhaseComponent::CoTaskWriteDebugTexture), std::ref(engine), (uint32_t)safe_cast<uint32_t>(frameIndex * tasks.size() + i), std::move(task.texture->description.data), task.texture));
			finalStatus.store(TaskData::STATUS_BAKED, std::memory_order_release);
		}
	}

	// Commit bakes
	if (!bakeQueues.empty()) {
		render.PresentQueues(&bakeQueues[0], safe_cast<uint32_t>(bakeQueues.size()), IRender::PRESENT_EXECUTE_ALL);
	}
}

void PhaseComponent::CoTaskWriteDebugTexture(Engine& engine, uint32_t index, Bytes& data, const TShared<TextureResource>& texture) {
	if (!debugPath.empty()) {
		std::stringstream ss;
		ss << debugPath << "phase_" << index << ".png";
		uint64_t length;
		IStreamBase* stream = engine.interfaces.archive.Open(StdToUtf8(ss.str()), true, length);
		IRender::Resource::TextureDescription& description = texture->description;
		IImage& image = engine.interfaces.image;
		IRender::Resource::TextureDescription::Layout layout = (IRender::Resource::TextureDescription::Layout)description.state.layout;
		IRender::Resource::TextureDescription::Format format = (IRender::Resource::TextureDescription::Format)description.state.format;
		length = data.GetSize();

		if (layout == IRender::Resource::TextureDescription::DEPTH) {
			layout = IRender::Resource::TextureDescription::R;
			format = IRender::Resource::TextureDescription::UNSIGNED_BYTE;
			size_t count = description.dimension.x() * description.dimension.y();
			const float* base = (const float*)data.GetData();
			uint8_t* target = (uint8_t*)data.GetData();
			length = sizeof(uint8_t) * count;

			for (size_t i = 0; i < count; i++) {
				target[i] = (uint8_t)(Math::Clamp(base[i], 0.f, 1.0f) * 0xff);
			}
		}
		
		IImage::Image* png = image.Create(description.dimension.x(), description.dimension.y(), layout, format);
		void* buffer = image.GetBuffer(png);
		memcpy(buffer, data.GetData(), safe_cast<size_t>(length));
		image.Save(png, *stream, "png");
		image.Delete(png);
		// write png
		stream->Destroy();
	}
}

void PhaseComponent::ResolveTasks(Engine& engine) {
	// resolve finished tasks
	for (size_t k = 0; k < tasks.size(); k++) {
		TaskData& task = tasks[k];
		std::atomic<uint32_t>& finalStatus = reinterpret_cast<std::atomic<uint32_t>&>(task.status);
		if (task.status == TaskData::STATUS_BAKED) {
			// TODO: finish baked
			finalStatus.store(TaskData::STATUS_IDLE);
		} else if (task.status == TaskData::STATUS_ASSEMBLING) {
			if (task.pendingCount == 0) {
				// Commit draw calls.
				IRender::Queue* queue = task.renderQueue;
				IRender& render = engine.interfaces.render;

				for (size_t i = 0; i < task.warpData.size(); i++) {
					WarpData& warpData = task.warpData[i];
					std::vector<IDataUpdater*>& dataUpdaters = warpData.dataUpdaters;

					for (size_t k = 0; k < dataUpdaters.size(); k++) {
						dataUpdaters[k]->Update(render, queue);
					}

					dataUpdaters.clear();
					WarpData::InstanceGroupMap& groups = warpData.instanceGroups;

					for (WarpData::InstanceGroupMap::iterator it = groups.begin(); it != groups.end(); ++it) {
						InstanceGroup& group = (*it).second;
						if (group.drawCallDescription.shaderResource == nullptr || group.instanceCount == 0) continue;

						std::vector<IRender::Resource*> buffers;
						buffers.reserve(group.instancedData.size());

						for (size_t k = 0; k < group.instancedData.size(); k++) {
							Bytes& data = group.instancedData[k];
							assert(!data.Empty());
							if (!data.Empty()) {
								IRender::Resource* buffer = render.CreateResource(render.GetQueueDevice(queue), IRender::Resource::RESOURCE_BUFFER);
								IRender::Resource::BufferDescription desc;
								desc.format = IRender::Resource::BufferDescription::FLOAT;
								desc.usage = IRender::Resource::BufferDescription::INSTANCED;
								desc.component = safe_cast<uint8_t>(data.GetSize() / (group.instanceCount * sizeof(float)));
								desc.data.Resize(data.GetViewSize());
								desc.data.Import(0, data);
								render.UploadResource(queue, buffer, &desc);

								// assign instanced buffer	
								// assert(group.drawCallDescription.bufferResources[output.slot].buffer == nullptr);
								IRender::Resource::DrawCallDescription::BufferRange& bufferRange = k < sizeof(group.drawCallDescription.bufferResources) / sizeof(group.drawCallDescription.bufferResources[0]) ? group.drawCallDescription.bufferResources[k] : group.drawCallDescription.extraBufferResources[k - sizeof(group.drawCallDescription.bufferResources) / sizeof(group.drawCallDescription.bufferResources[0])];
								bufferRange.buffer = buffer;
								buffers.emplace_back(buffer);
							}
						}

						group.drawCallDescription.instanceCounts.x() = group.instanceCount;

						assert(PassBase::ValidateDrawCall(group.drawCallDescription));
						IRender::Resource* drawCall = render.CreateResource(render.GetQueueDevice(queue), IRender::Resource::RESOURCE_DRAWCALL);
						IRender::Resource::DrawCallDescription dc = group.drawCallDescription; // make copy
						render.UploadResource(queue, drawCall, &dc);
						render.ExecuteResource(queue, drawCall);

						// cleanup at current frame
						render.DeleteResource(queue, drawCall);

						for (size_t n = 0; n < buffers.size(); n++) {
							render.DeleteResource(queue, buffers[n]);
						}

						group.Reset(); // for next reuse
					}

					for (size_t n = 0; n < warpData.runtimeResources.size(); n++) {
						render.DeleteResource(queue, warpData.runtimeResources[n]);
					}

					warpData.runtimeResources.clear();
					warpData.bytesCache.Reset();
				}

				finalStatus.store(TaskData::STATUS_ASSEMBLED, std::memory_order_release);
			}
		}
	}
}

void PhaseComponent::TaskAssembleTaskBounce(Engine& engine, TaskData& task, const UpdatePointBounce& bakePoint) {
	IRender& render = engine.interfaces.render;
	assert(task.status == TaskData::STATUS_DISPATCHED);
	IRender::Resource::RenderTargetDescription desc;

	IRender::Resource::RenderTargetDescription::Storage storage;
	const Phase& fromPhase = phases[bakePoint.fromPhaseIndex];
	Phase& toPhase = phases[bakePoint.toPhaseIndex];
	storage.resource = toPhase.irradiance->GetRenderResource();
	storage.loadOp = IRender::Resource::RenderTargetDescription::DISCARD;
	storage.storeOp = IRender::Resource::RenderTargetDescription::DEFAULT;
	desc.colorStorages.emplace_back(storage);
	desc.depthStorage.loadOp = IRender::Resource::RenderTargetDescription::DISCARD; // TODO:
	desc.depthStorage.storeOp = IRender::Resource::RenderTargetDescription::DISCARD;
	// task.texture = toPhase.irradiance;
	task.texture = nullptr;

	// TODO: fill params
	MultiHashTraceFS& fs = static_cast<MultiHashTracePass&>(toPhase.tracePipeline->GetPass()).shaderMultiHashTrace;

	// changing state
	render.UploadResource(task.renderQueue, task.renderTarget, &desc);
	render.ExecuteResource(task.renderQueue, stateResource);
	render.ExecuteResource(task.renderQueue, task.renderTarget);

	// encode draw call
	std::vector<IRender::Resource*> placeholders;
	PassBase::Updater& updater = toPhase.tracePipeline->GetPassUpdater();
	std::vector<Bytes> bufferData;
	updater.Capture(toPhase.drawCallDescription, bufferData, 1 << IRender::Resource::BufferDescription::UNIFORM);
	updater.Update(render, task.renderQueue, toPhase.drawCallDescription, placeholders, bufferData, 1 << IRender::Resource::BufferDescription::UNIFORM);
	assert(placeholders.empty());
	
	render.UploadResource(task.renderQueue, toPhase.drawCallResource, &toPhase.drawCallDescription);
	render.ExecuteResource(task.renderQueue, toPhase.drawCallResource);
}

void PhaseComponent::CoTaskAssembleTaskShadow(Engine& engine, TaskData& task, const UpdatePointShadow& bakePoint) {
	IRender& render = engine.interfaces.render;
	assert(task.status == TaskData::STATUS_DISPATCHED);
	IRender::Resource::RenderTargetDescription desc;

	const Shadow& shadow = shadows[bakePoint.shadowIndex];
	desc.depthStorage.resource = shadow.shadow->GetRenderResource();
	IRender::Resource::RenderTargetDescription::Storage color;
	color.resource = emptyColorAttachment->GetRenderResource(); // Don't care
	color.loadOp = IRender::Resource::RenderTargetDescription::DISCARD;
	color.storeOp = IRender::Resource::RenderTargetDescription::DISCARD;
	desc.colorStorages.emplace_back(color);
	desc.depthStorage.loadOp = IRender::Resource::RenderTargetDescription::CLEAR;
	desc.depthStorage.storeOp = IRender::Resource::RenderTargetDescription::DEFAULT;

	render.UploadResource(task.renderQueue, task.renderTarget, &desc);
	render.ExecuteResource(task.renderQueue, stateShadowResource);
	render.ExecuteResource(task.renderQueue, task.renderTarget);

	task.pipeline = shadowPipeline();
	task.texture = shadow.shadow;
	// task.texture = nullptr;
	Collect(engine, task, shadow.viewProjectionMatrix);
}

void PhaseComponent::CompleteCollect(Engine& engine, TaskData& taskData) {}

void PhaseComponent::Collect(Engine& engine, TaskData& taskData, const MatrixFloat4x4& viewProjectionMatrix) {
	PerspectiveCamera camera;
	CaptureData captureData;
	camera.UpdateCaptureData(captureData, viewProjectionMatrix);
	assert(rootEntity->GetWarpIndex() == GetWarpIndex());
	WorldInstanceData instanceData;
	instanceData.worldMatrix = viewProjectionMatrix;
	std::atomic<uint32_t>& finalStatus = reinterpret_cast<std::atomic<uint32_t>&>(taskData.status);
	finalStatus.store(TaskData::STATUS_ASSEMBLING, std::memory_order_release);

	std::atomic_thread_fence(std::memory_order_acquire);
	CollectComponentsFromEntity(engine, taskData, instanceData, captureData, rootEntity);
}

void PhaseComponent::CoTaskAssembleTaskSetup(Engine& engine, TaskData& task, const UpdatePointSetup& bakePoint) {
	IRender& render = engine.interfaces.render;
	assert(task.status == TaskData::STATUS_DISPATCHED);
	IRender::Resource::RenderTargetDescription desc;
	const Phase& phase = phases[bakePoint.phaseIndex];

	TextureResource* rt[] = {
		phase.baseColorOcclusion(),
		phase.normalRoughnessMetallic()
	};

	for (size_t k = 0; k < sizeof(rt) / sizeof(rt[0]); k++) {
		IRender::Resource::RenderTargetDescription::Storage storage;
		storage.resource = rt[k]->GetRenderResource();
		desc.colorStorages.emplace_back(storage);
	}

	render.UploadResource(task.renderQueue, task.renderTarget, &desc);
	render.ExecuteResource(task.renderQueue, stateResource);
	render.ExecuteResource(task.renderQueue, task.renderTarget);

	task.texture = phase.baseColorOcclusion;
	task.pipeline = setupPipeline();
	task.camera = phase.camera;
	Collect(engine, task, phase.viewProjectionMatrix);
}

void PhaseComponent::DispatchTasks(Engine& engine) {
	size_t n = 0;
	Kernel& kernel = engine.GetKernel();
	ThreadPool& threadPool = kernel.GetThreadPool();

	for (size_t i = 0; i < tasks.size(); i++) {
		TaskData& task = tasks[n];
		std::atomic<uint32_t>& finalStatus = reinterpret_cast<std::atomic<uint32_t>&>(task.status);
		if (task.status == TaskData::STATUS_START) {
			finalStatus.store(TaskData::STATUS_DISPATCHED, std::memory_order_relaxed);

			if (!bakePointShadows.empty()) {
				const UpdatePointShadow& shadow = bakePointShadows.top();
				threadPool.Push(CreateCoTaskContextFree(kernel, Wrap(this, &PhaseComponent::CoTaskAssembleTaskShadow), std::ref(engine), std::ref(task), shadow));
				bakePointShadows.pop();
			} else if (!bakePointSetups.empty()) {
				const UpdatePointSetup& setup = bakePointSetups.top();
				threadPool.Push(CreateCoTaskContextFree(kernel, Wrap(this, &PhaseComponent::CoTaskAssembleTaskSetup), std::ref(engine), std::ref(task), setup));
				bakePointSetups.pop();
			} else if (!bakePointBounces.empty()) {
				const UpdatePointBounce& bounce = bakePointBounces.top();
				TaskAssembleTaskBounce(engine, task, bounce);
				bakePointBounces.pop();
			} else {
				finalStatus.store(TaskData::STATUS_DISPATCHED, std::memory_order_relaxed);
			}
		}
	}
}

void PhaseComponent::CollectRenderableComponent(Engine& engine, TaskData& taskData, RenderableComponent* renderableComponent, WorldInstanceData& instanceData) {
	IRender& render = engine.interfaces.render;
	IRender::Queue* queue = taskData.renderQueue;
	uint32_t currentWarpIndex = engine.GetKernel().GetCurrentWarpIndex();
	WarpData& warpData = taskData.warpData[currentWarpIndex == ~(uint32_t)0 ? GetWarpIndex() : currentWarpIndex];

	InstanceKey key;
	key.renderKey = (size_t)renderableComponent;
	InstanceGroup& first = warpData.instanceGroups[key];
	std::vector<IRender::Resource*> textureResources;
	std::vector<IRender::Resource::DrawCallDescription::BufferRange> bufferResources;

	if (first.drawCallDescription.shaderResource != nullptr) {
		size_t i = 0;
		while (true) {
			InstanceKey key;
			key.renderKey = (size_t)renderableComponent + i++;
			InstanceGroup& group = warpData.instanceGroups[key];
			if (group.drawCallDescription.shaderResource == nullptr)
				break;

			group.instanceUpdater->Snapshot(group.instancedData, bufferResources, textureResources, instanceData, &warpData.bytesCache);
			group.instanceCount++;
		}
	} else {
		IDrawCallProvider::InputRenderData inputRenderData(0.0f, taskData.pipeline);
		IDrawCallProvider::DrawCallAllocator allocator(&warpData.bytesCache);
		std::vector<IDrawCallProvider::OutputRenderData, IDrawCallProvider::DrawCallAllocator> drawCalls(allocator);
		renderableComponent->CollectDrawCalls(drawCalls, inputRenderData, warpData.bytesCache);
		assert(drawCalls.size() < sizeof(RenderableComponent) - 1);

		for (size_t i = 0; i < drawCalls.size(); i++) {
			InstanceKey key;
			key.renderKey = (size_t)renderableComponent + i;
			InstanceGroup& group = warpData.instanceGroups[key];
			
			if (group.instanceCount == 0) {
				IDrawCallProvider::OutputRenderData& drawCall = drawCalls[i];
				std::binary_insert(warpData.dataUpdaters, drawCall.dataUpdater);
				group.drawCallDescription = std::move(drawCall.drawCallDescription);

				std::map<ShaderResource*, WarpData::GlobalBufferItem>::iterator ip = warpData.worldGlobalBufferMap.find(drawCall.shaderResource());
				PassBase::Updater& updater = drawCall.shaderResource->GetPassUpdater();

				if (ip == warpData.worldGlobalBufferMap.end()) {
					ip = warpData.worldGlobalBufferMap.insert(std::make_pair(drawCall.shaderResource(), WarpData::GlobalBufferItem())).first;

					taskData.worldGlobalData.Export(ip->second.globalUpdater, updater);
					instanceData.Export(ip->second.instanceUpdater, updater);

					std::vector<Bytes> s;
					std::vector<IRender::Resource::DrawCallDescription::BufferRange> bufferResources;
					std::vector<IRender::Resource*> textureResources;
					ip->second.globalUpdater.Snapshot(s, bufferResources, textureResources, taskData.worldGlobalData);
					ip->second.buffers.resize(updater.GetBufferCount());

					for (size_t i = 0; i < s.size(); i++) {
						Bytes& data = s[i];
						if (!data.Empty()) {
							IRender::Resource::BufferDescription desc;
							desc.usage = IRender::Resource::BufferDescription::UNIFORM;
							desc.component = 4;
							desc.dynamic = 1;
							desc.format = IRender::Resource::BufferDescription::FLOAT;
							desc.data = std::move(data);
							IRender::Resource* res = render.CreateResource(render.GetQueueDevice(queue), IRender::Resource::RESOURCE_BUFFER);
							render.UploadResource(queue, res, &desc);
							ip->second.buffers[i] = res;
							warpData.runtimeResources.emplace_back(res);
						}
					}
				}

				for (size_t n = 0; n < group.drawCallDescription.bufferCount; n++) {
					IRender::Resource::DrawCallDescription::BufferRange& bufferRange = n < sizeof(group.drawCallDescription.bufferResources) / sizeof(group.drawCallDescription.bufferResources[0]) ? group.drawCallDescription.bufferResources[n] : group.drawCallDescription.extraBufferResources[n - sizeof(group.drawCallDescription.bufferResources) / sizeof(group.drawCallDescription.bufferResources[0])];
					if (ip->second.buffers[n] != nullptr) {
						bufferRange.buffer = ip->second.buffers[n];
						bufferRange.offset = bufferRange.length = 0;
					}
				}

				group.instanceUpdater = &ip->second.instanceUpdater;
			}

			group.instanceUpdater->Snapshot(group.instancedData, bufferResources, textureResources, instanceData, &warpData.bytesCache);
			group.instanceCount++;
		}
	}
}

void PhaseComponent::CompleteUpdateLights(Engine& engine, std::vector<LightElement>& elements) {
	MatrixFloat4x4 projectionMatrix = Math::MatrixOrtho(range);
	bakePointShadows = std::stack<UpdatePointShadow>();
	shadows.resize(elements.size());

	// generate shadow tasks
	for (size_t i = 0; i < elements.size(); i++) {
		Shadow& shadow = shadows[i];
		const LightElement& lightElement = elements[i];
		if (!shadow.shadow) {
			shadow.shadow = engine.snowyStream.CreateReflectedResource(UniqueType<TextureResource>(), ResourceBase::GenerateLocation("PhaseShadow", &shadow), false, ResourceBase::RESOURCE_VIRTUAL);
			shadow.shadow->description.state.attachment = true;
			shadow.shadow->description.dimension = UShort3(resolution.x(), resolution.y(), 1);
			shadow.shadow->description.state.format = IRender::Resource::TextureDescription::FLOAT;
			shadow.shadow->description.state.layout = IRender::Resource::TextureDescription::DEPTH;
			shadow.shadow->Flag().fetch_or(Tiny::TINY_MODIFIED, std::memory_order_release);
			shadow.shadow->GetResourceManager().InvokeUpload(shadow.shadow(), renderQueue);
		}

		assert(hostEntity != nullptr);
		TransformComponent* transformComponent = hostEntity->GetUniqueComponent(UniqueType<TransformComponent>());
		Float3 center;
		if (transformComponent != nullptr) {
			center = transformComponent->GetQuickTranslation();
		}

		shadow.lightElement = lightElement;
		Float3 view = (Float3)lightElement.position;
		Float3 dir = -view;
		if (Math::SquareLength(dir) < 1e-6) {
			dir = Float3(0, 0, -1);
		}

		Float3 up(RandFloat(), RandFloat(), RandFloat());

		shadow.viewProjectionMatrix = Math::MatrixLookAt(view, dir, up) * projectionMatrix;
		UpdatePointShadow bakePointShadow;
		bakePointShadow.shadowIndex = safe_cast<uint32_t>(i);
		bakePointShadows.push(bakePointShadow);
	}

	// generate setup tasks
	bakePointSetups = std::stack<UpdatePointSetup>();
	for (size_t j = 0; j < phases.size(); j++) {
		UpdatePointSetup bakePoint;
		bakePoint.phaseIndex = safe_cast<uint32_t>(j);
		bakePointSetups.push(bakePoint);
	}

	// stop all bounces
	bakePointBounces = std::stack<UpdatePointBounce>();
}

PhaseComponent::LightCollector::LightCollector(PhaseComponent* component) : phaseComponent(component) {}

void PhaseComponent::LightCollector::CollectComponents(Engine& engine, TaskData& task, const WorldInstanceData& inst, const CaptureData& captureData, Entity* entity) {
	const std::vector<Component*>& components = entity->GetComponents();
	TransformComponent* transformComponent = entity->GetUniqueComponent(UniqueType<TransformComponent>());
	if (transformComponent != nullptr) {
		const Float3Pair& localBoundingBox = transformComponent->GetLocalBoundingBox();
		if (!captureData(localBoundingBox))
			return;
	}

	WorldInstanceData instanceData;
	instanceData.worldMatrix = transformComponent != nullptr ? transformComponent->GetTransform() * inst.worldMatrix : inst.worldMatrix;

	for (size_t i = 0; i < components.size(); i++) {
		Component* component = components[i];
		if (component != nullptr) {
			uint32_t flag = component->GetEntityFlagMask();
			std::atomic<uint32_t>& counter = reinterpret_cast<std::atomic<uint32_t>&>(task.pendingCount);
			if (flag & Entity::ENTITY_HAS_SPACE) {
				counter.fetch_add(1, std::memory_order_release);
				CollectComponentsFromSpace(engine, task, instanceData, captureData, static_cast<SpaceComponent*>(component));
			} else if (flag & Entity::ENTITY_HAS_RENDERCONTROL) {
				LightComponent* lightComponent = component->QueryInterface(UniqueType<LightComponent>());
				if (lightComponent != nullptr) {
					LightElement element;
					const MatrixFloat4x4& worldMatrix = instanceData.worldMatrix;
					if (lightComponent->Flag().load(std::memory_order_relaxed) & LightComponent::LIGHTCOMPONENT_DIRECTIONAL) {
						element.position = Float4(-worldMatrix(2, 0), -worldMatrix(2, 1), -worldMatrix(2, 2), 0);
					} else {
						// Only directional light by now
						element.position = Float4(worldMatrix(3, 0), worldMatrix(3, 1), worldMatrix(3, 2), 1);
					}

					const Float3& color = lightComponent->GetColor();
					element.colorAttenuation = Float4(color.x(), color.y(), color.z(), lightComponent->GetAttenuation());
					task.warpData[engine.GetKernel().GetCurrentWarpIndex()].lightElements.emplace_back(element);
				}
			}
		}
	}
}

void PhaseComponent::LightCollector::CompleteCollect(Engine& engine, TaskData& taskData) {
	std::vector<LightElement> lightElements;
	for (size_t i = 0; i < taskData.warpData.size(); i++) {
		LightConfig::WarpData& warpData = taskData.warpData[i];
		std::copy(warpData.lightElements.begin(), warpData.lightElements.end(), std::back_inserter(lightElements));
	}

	engine.GetKernel().QueueRoutine(phaseComponent, CreateTaskContextFree(Wrap(phaseComponent, &PhaseComponent::CompleteUpdateLights), std::ref(engine), lightElements));
	delete &taskData;
	phaseComponent->ReleaseObject();
}

void PhaseComponent::LightCollector::InvokeCollect(Engine& engine, Entity* entity) {
	LightConfig::TaskData* taskData = new LightConfig::TaskData(engine.GetKernel().GetWarpCount());
	phaseComponent->ReferenceObject();
	LightConfig::WorldInstanceData worldInstance;
	worldInstance.worldMatrix = MatrixFloat4x4::Identity();
	LightConfig::CaptureData captureData;

	std::atomic_thread_fence(std::memory_order_acquire);
	CollectComponentsFromEntity(engine, *taskData, worldInstance, captureData, entity);
}

void PhaseComponent::Update(Engine& engine, const Float3& center) {
	// generate bake points
	srand((int)time(nullptr));
	bakePointSetups = std::stack<UpdatePointSetup>();

	PerspectiveCamera camera;
	camera.nearPlane = 0.01f;
	camera.farPlane = 1000.0f;
	camera.aspect = 1.0f;
	camera.fov = PI / 2;

	MatrixFloat4x4 projectionMatrix = Math::MatrixPerspective(camera.fov, camera.aspect, camera.nearPlane, camera.farPlane);

	// Adjust phases?
	for (size_t i = 0; i < phases.size(); i++) {
		float theta = RandFloat() * 2 * PI;
		float phi = RandFloat() * PI;

		Float3 view = range * Float3(cos(theta), sin(theta), cos(phi)) * 4.0f;
		Float3 dir = -view;
		Float3 up(RandFloat(), RandFloat(), RandFloat());

		phases[i].camera = camera;
		phases[i].projectionMatrix = projectionMatrix;
		phases[i].viewProjectionMatrix = Math::MatrixLookAt(view + center, dir + center, up) * projectionMatrix;
	}

	lightCollector.InvokeCollect(engine, rootEntity);
}

void PhaseComponent::CollectComponents(Engine& engine, TaskData& task, const WorldInstanceData& inst, const CaptureData& captureData, Entity* entity) {
	Tiny::FLAG rootFlag = entity->Flag().load(std::memory_order_relaxed);
	const Float3Pair& box = entity->GetKey();
	if ((rootFlag & Tiny::TINY_ACTIVATED) && captureData(box)) {
		TransformComponent* transformComponent = entity->GetUniqueComponent(UniqueType<TransformComponent>());
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
					if (!(renderableComponent->Flag().load(std::memory_order_relaxed) & RenderableComponent::RENDERABLECOMPONENT_CAMERAVIEW)) {
						CollectRenderableComponent(engine, task, renderableComponent, instanceData);
					}
				}
			} else if (component->GetEntityFlagMask() & Entity::ENTITY_HAS_SPACE) {
				std::atomic<uint32_t>& counter = reinterpret_cast<std::atomic<uint32_t>&>(task.pendingCount);
				counter.fetch_add(1, std::memory_order_release);
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
}

const std::vector<PhaseComponent::Phase>& PhaseComponent::GetPhases() const {
	return phases;
}

void PhaseComponent::UpdateRenderFlow(Engine& engine) {
	RenderPort* renderPort = renderFlowComponent->BeginPort(lightPhaseViewPortName);
	if (renderPort != nullptr) {
		RenderPortPhaseLightView* phaseLightView = renderPort->QueryInterface(UniqueType<RenderPortPhaseLightView>());
		if (phaseLightView != nullptr) {
			// Update phases
			for (size_t i = 0; i < phases.size(); i++) {
				phaseLightView->phases[i] = phases[i];
			}
		}

		renderFlowComponent->EndPort(renderPort);
	}
}

void PhaseComponent::BindRootEntity(Engine& engine, Entity* entity) {
	if (rootEntity != nullptr) {
		// free last listener
		rootEntity->RemoveComponent(engine, this);
	}

	rootEntity = entity;

	if (entity != nullptr) {
		entity->AddComponent(engine, this); // weak component
	}
}

void PhaseComponent::SetDebugMode(const String& path) {
	debugPath = path;
}

