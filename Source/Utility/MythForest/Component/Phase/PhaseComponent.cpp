#include "PhaseComponent.h"
#include "../Light/LightComponent.h"
#include "../Space/SpaceComponent.h"
#include "../Renderable/RenderableComponent.h"
#include "../Transform/TransformComponent.h"
#include "../Explorer/ExplorerComponent.h"
#include <ctime>

const float PI = 3.1415926f;
static inline float RandFloat() {
	return (float)rand() / RAND_MAX;
}

using namespace PaintsNow;
using namespace PaintsNow::NsMythForest;
using namespace PaintsNow::NsSnowyStream;

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

PhaseComponent::PhaseComponent(TShared<RenderFlowComponent> renderFlow, const String& portName) : hostEntity(nullptr), maxTracePerTick(8), renderQueue(nullptr), clearResource(nullptr), stateResource(nullptr), stateShadowResource(nullptr), range(32, 32, 32), resolution(512, 512), lightCollector(this), renderFlowComponent(renderFlow), lightPhaseViewPortName(portName), rootEntity(nullptr) {}

PhaseComponent::~PhaseComponent() {}

void PhaseComponent::Initialize(Engine& engine, Entity* entity) {
	if (rootEntity != entity) { // Set Host?
		Component::Initialize(engine, entity);
		assert(hostEntity == nullptr);
		assert(renderQueue == nullptr);
		hostEntity = entity;

		String location = ResourceBase::GenerateLocation("PhaseEmptyColorAttachment", (void*)(((size_t)resolution.x() << 16) | resolution.y()));
		emptyColorAttachment = engine.snowyStream.CreateReflectedResource(UniqueType<TextureResource>(), location);

		if (!emptyColorAttachment) {
			emptyColorAttachment = engine.snowyStream.CreateReflectedResource(UniqueType<TextureResource>(), location, false, 0, nullptr);
			emptyColorAttachment->description.dimension = UShort3(resolution.x(), resolution.y(), 1);
			emptyColorAttachment->description.state.format = IRender::Resource::TextureDescription::UNSIGNED_BYTE;
			emptyColorAttachment->description.state.layout = IRender::Resource::TextureDescription::R;
			emptyColorAttachment->GetResourceManager().InvokeUpload(emptyColorAttachment(), engine.snowyStream.GetResourceQueue());
		}

		SnowyStream& snowyStream = engine.snowyStream;
		const String path = "[Runtime]/MeshResource/StandardSquare";
		quadMeshResource = snowyStream.CreateReflectedResource(UniqueType<NsSnowyStream::MeshResource>(), path, true, 0, nullptr);

		tracePipeline = snowyStream.CreateReflectedResource(UniqueType<ShaderResource>(), ShaderResource::GetShaderPathPrefix() + UniqueType<MultiHashTracePass>::Get()->GetSubName())->QueryInterface(UniqueType<ShaderResourceImpl<MultiHashTracePass> >());
		setupPipeline = snowyStream.CreateReflectedResource(UniqueType<ShaderResource>(), ShaderResource::GetShaderPathPrefix() + UniqueType<MultiHashSetupPass>::Get()->GetSubName())->QueryInterface(UniqueType<ShaderResourceImpl<MultiHashSetupPass> >());
		shadowPipeline = snowyStream.CreateReflectedResource(UniqueType<ShaderResource>(), ShaderResource::GetShaderPathPrefix() + UniqueType<ConstMapPass>::Get()->GetSubName())->QueryInterface(UniqueType<ShaderResourceImpl<ConstMapPass> >());

		IRender& render = engine.interfaces.render;
		IRender::Device* device = engine.snowyStream.GetRenderDevice();
		renderQueue = render.CreateQueue(device);

		clearResource = render.CreateResource(renderQueue, IRender::Resource::RESOURCE_CLEAR);
		IRender::Resource::ClearDescription clear;
		clear.clearColorBit = IRender::Resource::ClearDescription::DISCARD_LOAD;
		clear.clearDepthBit = IRender::Resource::ClearDescription::DISCARD_LOAD | IRender::Resource::ClearDescription::DISCARD_STORE;
		clear.clearStencilBit = IRender::Resource::ClearDescription::DISCARD_LOAD | IRender::Resource::ClearDescription::DISCARD_STORE;
		render.UploadResource(renderQueue, clearResource, &clear);

		clearShadowResource = render.CreateResource(renderQueue, IRender::Resource::RESOURCE_CLEAR);
		clear.clearColorBit = IRender::Resource::ClearDescription::DISCARD_LOAD | IRender::Resource::ClearDescription::DISCARD_STORE;
		clear.clearDepthBit = 0;
		clear.clearStencilBit = IRender::Resource::ClearDescription::DISCARD_LOAD | IRender::Resource::ClearDescription::DISCARD_STORE;
		render.UploadResource(renderQueue, clearShadowResource, &clear);

		stateResource = render.CreateResource(renderQueue, IRender::Resource::RESOURCE_RENDERSTATE);
		IRender::Resource::RenderStateDescription state;
		state.cull = 1;
		state.fill = 1;
		state.alphaBlend = 0;
		state.colorWrite = 1;
		state.depthTest = IRender::Resource::RenderStateDescription::DISABLED;
		state.depthWrite = 0;
		state.stencilTest = IRender::Resource::RenderStateDescription::DISABLED;
		state.stencilWrite = 0;
		render.UploadResource(renderQueue, stateResource, &state);

		state.cull = 1;
		state.fill = 1;
		state.alphaBlend = 0;
		state.colorWrite = 0;
		state.depthTest = IRender::Resource::RenderStateDescription::GREATER_EQUAL;
		state.depthWrite = 1;
		state.stencilTest = IRender::Resource::RenderStateDescription::DISABLED;
		state.stencilWrite = 0;
		state.cullFrontFace = 1;
		stateShadowResource = render.CreateResource(renderQueue, IRender::Resource::RESOURCE_RENDERSTATE);
		render.UploadResource(renderQueue, stateShadowResource, &state);
	}
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

		render.DeleteResource(queue, clearResource);
		render.DeleteResource(queue, clearShadowResource);
		render.DeleteResource(queue, stateResource);
		render.DeleteResource(queue, stateShadowResource);
		render.DeleteQueue(renderQueue);

		clearResource = clearShadowResource = stateResource = stateShadowResource = nullptr;
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
		task.renderTarget = render.CreateResource(task.renderQueue, IRender::Resource::RESOURCE_RENDERTARGET);
	}

	// prepare uniform buffers for tracing
	phases.resize(phaseCount);
	for (size_t i = 0; i < phases.size(); i++) {
		Phase& phase = phases[i];

		std::vector<Bytes> bufferData;
		phase.tracePipeline.Reset(static_cast<ShaderResourceImpl<MultiHashTracePass>*>(tracePipeline->Clone()));
		phase.tracePipeline->GetPassUpdater().Capture(phase.drawCallDescription, bufferData, 1 << IRender::Resource::BufferDescription::UNIFORM);
		phase.tracePipeline->GetPassUpdater().Update(render, renderQueue, phase.drawCallDescription, phase.uniformBuffers, bufferData, 1 << IRender::Resource::BufferDescription::UNIFORM);
		phase.drawCallResource = render.CreateResource(renderQueue, IRender::Resource::RESOURCE_DRAWCALL);

		phase.depth = engine.snowyStream.CreateReflectedResource(UniqueType<TextureResource>(), ResourceBase::GenerateLocation("PhaseDepth", &phase), false, 0, nullptr);
		phase.depth->description.dimension = UShort3(resolution.x(), resolution.y(), 1);
		phase.depth->description.state.format = IRender::Resource::TextureDescription::FLOAT;
		phase.depth->description.state.layout = IRender::Resource::TextureDescription::DEPTH;
		phase.depth->GetResourceManager().InvokeUpload(phase.depth(), renderQueue);

		phase.irradiance = engine.snowyStream.CreateReflectedResource(UniqueType<TextureResource>(), ResourceBase::GenerateLocation("PhaseIrradiance", &phase), false, 0, nullptr);
		phase.irradiance->description.dimension = UShort3(resolution.x(), resolution.y(), 1);
		phase.irradiance->description.state.format = IRender::Resource::TextureDescription::HALF_FLOAT;

		phase.irradiance->description.state.layout = IRender::Resource::TextureDescription::RGBA;
		phase.irradiance->GetResourceManager().InvokeUpload(phase.irradiance(), renderQueue);

		phase.baseColorOcclusion = engine.snowyStream.CreateReflectedResource(UniqueType<TextureResource>(), ResourceBase::GenerateLocation("PhaseBaseColorOcclusion", &phase), false, 0, nullptr);
		phase.baseColorOcclusion->description.dimension = UShort3(resolution.x(), resolution.y(), 1);
		phase.baseColorOcclusion->description.state.format = IRender::Resource::TextureDescription::UNSIGNED_BYTE;
		phase.baseColorOcclusion->description.state.layout = IRender::Resource::TextureDescription::RGBA;
		phase.baseColorOcclusion->GetResourceManager().InvokeUpload(phase.baseColorOcclusion(), renderQueue);

		phase.normalRoughnessMetallic = engine.snowyStream.CreateReflectedResource(UniqueType<TextureResource>(), ResourceBase::GenerateLocation("PhaseNormalRoughness", &phase), false, 0, nullptr);
		phase.normalRoughnessMetallic->description.dimension = UShort3(resolution.x(), resolution.y(), 1);
		phase.normalRoughnessMetallic->description.state.format = IRender::Resource::TextureDescription::UNSIGNED_BYTE;
		phase.normalRoughnessMetallic->description.state.layout = IRender::Resource::TextureDescription::RGBA;
		phase.normalRoughnessMetallic->GetResourceManager().InvokeUpload(phase.normalRoughnessMetallic(), renderQueue);

		// create noise texture
		phase.noiseTexture = engine.snowyStream.CreateReflectedResource(UniqueType<TextureResource>(), ResourceBase::GenerateLocation("PhaseNoise", &phase), false, 0, nullptr);
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
		TAtomic<uint32_t>& finalStatus = reinterpret_cast<TAtomic<uint32_t>&>(task.status);
		if (task.status == TaskData::STATUS_IDLE) {
			finalStatus.store(TaskData::STATUS_START, std::memory_order_release);
		} else if (task.status == TaskData::STATUS_ASSEMBLED) {
			uint32_t status = TaskData::STATUS_BAKED;
			if (!debugPath.empty() && task.texture) {
				render.RequestDownloadResource(task.renderQueue, task.texture->GetTexture(), &task.texture->description);
				status = TaskData::STATUS_DOWNLOADED;
			}

			// engine.mythForest.StartCaptureFrame("cap", "");
			render.YieldQueue(task.renderQueue);
			bakeQueues.emplace_back(task.renderQueue);
			finalStatus.store(status, std::memory_order_release);
			// render.PresentQueues(&task.renderQueue, 1, IRender::PRESENT_EXECUTE_ALL);
			// engine.mythForest.EndCaptureFrame();
		} else if (task.status == TaskData::STATUS_DOWNLOADED) {
			render.CompleteDownloadResource(task.renderQueue, task.texture->GetTexture());
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

void PhaseComponent::CoTaskWriteDebugTexture(Engine& engine, uint32_t index, Bytes& data, TShared<NsSnowyStream::TextureResource> texture) {
	if (!debugPath.empty()) {
		std::stringstream ss;
		ss << debugPath << "phase_" << index << ".png";
		size_t length;
		IStreamBase* stream = engine.interfaces.archive.Open(ss.str(), true, length);
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
				target[i] = (uint8_t)(Clamp(base[i], 0.f, 1.0f) * 0xff);
			}
		}
		
		IImage::Image* png = image.Create(description.dimension.x(), description.dimension.y(), layout, format);
		void* buffer = image.GetBuffer(png);
		memcpy(buffer, data.GetData(), length);
		image.Save(png, *stream, "png");
		image.Delete(png);
		// write png
		stream->ReleaseObject();
	}
}

void PhaseComponent::ResolveTasks(Engine& engine) {
	// resolve finished tasks
	for (size_t k = 0; k < tasks.size(); k++) {
		TaskData& task = tasks[k];
		TAtomic<uint32_t>& finalStatus = reinterpret_cast<TAtomic<uint32_t>&>(task.status);
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
								const ZPassBase::Parameter& output = group.instanceUpdater->parameters[k];
								// instanceable.
								assert(output.slot < group.drawCallDescription.bufferResources.size());
								IRender::Resource* buffer = render.CreateResource(queue, IRender::Resource::RESOURCE_BUFFER);
								IRender::Resource::BufferDescription desc;
								desc.format = IRender::Resource::BufferDescription::FLOAT;
								desc.usage = IRender::Resource::BufferDescription::INSTANCED;
								desc.component = data.GetSize() / (group.instanceCount * sizeof(float));
								desc.data = std::move(data);
								render.UploadResource(queue, buffer, &desc);

								// assign instanced buffer	
								// assert(group.drawCallDescription.bufferResources[output.slot].buffer == nullptr);
								group.drawCallDescription.bufferResources[output.slot].buffer = buffer;
								buffers.emplace_back(buffer);
							}
						}

						group.drawCallDescription.instanceCounts.x() = group.instanceCount;

						if (ZPassBase::ValidateDrawCall(group.drawCallDescription)) {
							IRender::Resource* drawCall = render.CreateResource(queue, IRender::Resource::RESOURCE_DRAWCALL);
							IRender::Resource::DrawCallDescription dc = group.drawCallDescription; // make copy
							render.UploadResource(queue, drawCall, &dc);
							render.ExecuteResource(queue, drawCall);

							// cleanup at current frame
							render.DeleteResource(queue, drawCall);
						}

						for (size_t n = 0; n < buffers.size(); n++) {
							render.DeleteResource(queue, buffers[n]);
						}

						group.Reset(); // for next reuse
					}

					for (size_t n = 0; n < warpData.runtimeResources.size(); n++) {
						render.DeleteResource(queue, warpData.runtimeResources[n]);
					}

					warpData.runtimeResources.clear();
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
	storage.resource = toPhase.irradiance->GetTexture();
	desc.colorBufferStorages.emplace_back(std::move(storage));
	// task.texture = toPhase.irradiance;
	task.texture = nullptr;

	// TODO: fill params
	MultiHashTraceFS& fs = static_cast<MultiHashTracePass&>(toPhase.tracePipeline->GetPass()).shaderMultiHashTrace;

	// changing state
	render.UploadResource(task.renderQueue, task.renderTarget, &desc);
	render.ExecuteResource(task.renderQueue, task.renderTarget);
	render.ExecuteResource(task.renderQueue, stateResource);
	render.ExecuteResource(task.renderQueue, clearResource);

	// encode draw call
	std::vector<IRender::Resource*> placeholders;
	ZPassBase::Updater& updater = toPhase.tracePipeline->GetPassUpdater();
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
	desc.depthStencilStorage.resource = shadow.shadow->GetTexture();
	IRender::Resource::RenderTargetDescription::Storage color;
	color.resource = emptyColorAttachment->GetTexture(); // Don't care
	desc.colorBufferStorages.emplace_back(std::move(color));

	render.UploadResource(task.renderQueue, task.renderTarget, &desc);
	render.ExecuteResource(task.renderQueue, task.renderTarget);
	render.ExecuteResource(task.renderQueue, stateShadowResource);
	render.ExecuteResource(task.renderQueue, clearShadowResource);

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
	TAtomic<uint32_t>& finalStatus = reinterpret_cast<TAtomic<uint32_t>&>(taskData.status);
	finalStatus.store(TaskData::STATUS_ASSEMBLING, std::memory_order_release);

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
		storage.resource = rt[k]->GetTexture();
		desc.colorBufferStorages.emplace_back(std::move(storage));
	}

	render.UploadResource(task.renderQueue, task.renderTarget, &desc);
	render.ExecuteResource(task.renderQueue, task.renderTarget);
	render.ExecuteResource(task.renderQueue, stateResource);
	render.ExecuteResource(task.renderQueue, clearResource);

	task.texture = phase.baseColorOcclusion;
	task.pipeline = setupPipeline();
	Collect(engine, task, phase.viewProjectionMatrix);
}

void PhaseComponent::DispatchTasks(Engine& engine) {
	size_t n = 0;
	Kernel& kernel = engine.GetKernel();
	ThreadPool& threadPool = kernel.threadPool;

	for (size_t i = 0; i < tasks.size(); i++) {
		TaskData& task = tasks[n];
		TAtomic<uint32_t>& finalStatus = reinterpret_cast<TAtomic<uint32_t>&>(task.status);
		if (task.status == TaskData::STATUS_START) {
			finalStatus.store(TaskData::STATUS_DISPATCHED, std::memory_order_relaxed);

			if (!bakePointShadows.empty()) {
				const UpdatePointShadow& shadow = bakePointShadows.top();
				threadPool.Push(CreateCoTaskContextFree(kernel, Wrap(this, &PhaseComponent::CoTaskAssembleTaskShadow), std::ref(engine), std::ref(task), shadow));
				bakePointShadows.pop();
			} /*else if (!bakePointSetups.empty()) {
				const UpdatePointSetup& setup = bakePointSetups.top();
				threadPool.Push(CreateCoTaskContextFree(kernel, Wrap(this, &PhaseComponent::CoTaskAssembleTaskSetup), std::ref(engine), std::ref(task), setup));
				bakePointSetups.pop();
			} else if (!bakePointBounces.empty()) {
				const UpdatePointBounce& bounce = bakePointBounces.top();
				TaskAssembleTaskBounce(engine, task, bounce);
				bakePointBounces.pop();
			} else {
				finalStatus.store(TaskData::STATUS_DISPATCHED, std::memory_order_relaxed);
			}*/
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

			std::vector<Bytes> s;
			group.instanceUpdater->Snapshot(s, bufferResources, textureResources, instanceData);

			assert(s.size() == group.instancedData.size());
			for (size_t k = 0; k < s.size(); k++) {
				assert(!s[k].Empty());
				group.instancedData[k].Append(s[k]);
			}

			group.instanceCount++;
		}
	} else {
		NsSnowyStream::IDrawCallProvider::InputRenderData inputRenderData(0.0f, taskData.pipeline);
		std::vector<NsSnowyStream::IDrawCallProvider::OutputRenderData> drawCalls;
		renderableComponent->CollectDrawCalls(drawCalls, inputRenderData);
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
				ZPassBase::Updater& updater = drawCall.shaderResource->GetPassUpdater();

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
							IRender::Resource* res = render.CreateResource(queue, IRender::Resource::RESOURCE_BUFFER);
							render.UploadResource(queue, res, &desc);
							ip->second.buffers[i] = res;
							warpData.runtimeResources.emplace_back(res);
						}
					}
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
			} else {
				std::vector<Bytes> s;
				group.instanceUpdater->Snapshot(s, bufferResources, textureResources, instanceData);
				for (size_t k = 0; k < s.size(); k++) {
					assert(!s[k].Empty());
					group.instancedData[k].Append(s[k]);
				}
			}

			group.instanceCount++;
		}
	}
}

void PhaseComponent::CompleteUpdateLights(Engine& engine, std::vector<LightElement>& elements) {
	MatrixFloat4x4 projectionMatrix = Ortho(range);
	bakePointShadows = std::stack<UpdatePointShadow>();
	shadows.resize(elements.size());

	// generate shadow tasks
	for (size_t i = 0; i < elements.size(); i++) {
		Shadow& shadow = shadows[i];
		const LightElement& lightElement = elements[i];
		if (!shadow.shadow) {
			shadow.shadow = engine.snowyStream.CreateReflectedResource(UniqueType<TextureResource>(), ResourceBase::GenerateLocation("PhaseShadow", &shadow), false, 0, nullptr);
			shadow.shadow->description.dimension = UShort3(resolution.x(), resolution.y(), 1);
			shadow.shadow->description.state.format = IRender::Resource::TextureDescription::FLOAT;
			shadow.shadow->description.state.layout = IRender::Resource::TextureDescription::DEPTH;
			shadow.shadow->GetResourceManager().InvokeUpload(shadow.shadow(), renderQueue);
		}

		shadow.lightElement = lightElement;
		Float3 view = (Float3)lightElement.position;
		Float3 dir = view;
		if (dir.SquareLength() < 1e-6) {
			dir = Float3(0, 0, -1);
		}

		Float3 up(RandFloat(), RandFloat(), RandFloat());

		shadow.viewProjectionMatrix = LookAt(view, dir, up) * projectionMatrix;
		UpdatePointShadow bakePointShadow;
		bakePointShadow.shadowIndex = safe_cast<uint32_t>(i);
		bakePointShadows.push(std::move(bakePointShadow));
	}

	// generate setup tasks
	bakePointSetups = std::stack<UpdatePointSetup>();
	for (size_t j = 0; j < phases.size(); j++) {
		UpdatePointSetup bakePoint;
		bakePoint.phaseIndex = safe_cast<uint32_t>(j);
		bakePointSetups.push(std::move(bakePoint));
	}

	// stop all bounces
	bakePointBounces = std::stack<UpdatePointBounce>();
}

PhaseComponent::LightCollector::LightCollector(PhaseComponent* component) : phaseComponent(component) {}

void PhaseComponent::LightCollector::CollectComponents(Engine& engine, TaskData& task, const WorldInstanceData& inst, const CaptureData& captureData, Entity* entity) {
	const std::vector<Component*>& components = entity->GetComponents();
	TransformComponent* transformComponent = entity->GetUniqueComponent(UniqueType<TransformComponent>());
	WorldInstanceData instanceData;
	instanceData.worldMatrix = transformComponent != nullptr ? transformComponent->GetTransform() * inst.worldMatrix : inst.worldMatrix;

	for (size_t i = 0; i < components.size(); i++) {
		Component* component = components[i];
		if (component != nullptr) {
			uint32_t flag = component->GetEntityFlagMask();
			if (flag & Entity::ENTITY_HAS_SPACE) {
				CollectComponentsFromSpace(engine, task, instanceData, captureData, static_cast<SpaceComponent*>(component));
			} else if (flag & Entity::ENTITY_HAS_RENDERCONTROL) {
				LightComponent* lightComponent = component->QueryInterface(UniqueType<LightComponent>());
				if (lightComponent != nullptr) {
					LightElement element;
					const MatrixFloat4x4& worldMatrix = instanceData.worldMatrix;
					if (lightComponent->Flag() & LightComponent::LIGHTCOMPONENT_DIRECTIONAL) {
						element.position = Float4(-worldMatrix(2, 0), -worldMatrix(2, 1), -worldMatrix(2, 2), 0);
					} else {
						// Only directional light by now
						element.position = Float4(worldMatrix(3, 0), worldMatrix(3, 1), worldMatrix(3, 2), 1);
					}

					const Float3& color = lightComponent->GetColor();
					element.colorAttenuation = Float4(color.x(), color.y(), color.z(), lightComponent->GetAttenuation());
					task.warpData[engine.GetKernel().GetCurrentWarpIndex()].lightElements.emplace_back(std::move(element));
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
	LightConfig::CaptureData captureData;
	CollectComponentsFromEntity(engine, *taskData, worldInstance, captureData, entity);
}

void PhaseComponent::Update(Engine& engine, const Float3& center) {
	// generate bake points
	srand((int)time(nullptr));
	bakePointSetups = std::stack<UpdatePointSetup>();

	MatrixFloat4x4 projectionMatrix = Perspective(PI / 2, 1.0f, 0.01f, 1000.0f);

	// Adjust phases?
	for (size_t i = 0; i < phases.size(); i++) {
		float theta = RandFloat() * 2 * PI;
		float phi = RandFloat() * PI;

		Float3 view = range * Float3(cos(theta), sin(theta), cos(phi)) * 4.0f;
		Float3 dir = -view;
		Float3 up(RandFloat(), RandFloat(), RandFloat());

		phases[i].projectionMatrix = projectionMatrix;
		phases[i].viewProjectionMatrix = LookAt(view + center, dir + center, up) * projectionMatrix;
	}

	lightCollector.InvokeCollect(engine, rootEntity);
}

void PhaseComponent::CollectComponents(Engine& engine, TaskData& task, const WorldInstanceData& inst, const CaptureData& captureData, Entity* entity) {
	Tiny::FLAG rootFlag = entity->Flag().load(std::memory_order_acquire);
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
					CollectRenderableComponent(engine, task, renderableComponent, instanceData);
				}
			} else if (component->GetEntityFlagMask() & Entity::ENTITY_HAS_SPACE) {
				TAtomic<uint32_t>& counter = reinterpret_cast<TAtomic<uint32_t>&>(task.pendingCount);
				++counter;
				SpaceComponent* spaceComponent = static_cast<SpaceComponent*>(component);
				CollectComponentsFromSpace(engine, task, instanceData, captureData, spaceComponent);
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

