#include "PhaseComponent.h"
#include "../Light/LightComponent.h"
#include "../Space/SpaceComponent.h"
#include "../Renderable/RenderableComponent.h"
#include "../Transform/TransformComponent.h"
#include "../Explorer/ExplorerComponent.h"
#include "../../../../Core/Driver/Profiler/Optick/optick.h"
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

PhaseComponent::Phase::Phase() : shadowIndex(0), drawCallResource(nullptr) {}

TObject<IReflect>& PhaseComponentConfig::WorldGlobalData::operator () (IReflect& reflect) {
	BaseClass::operator () (reflect);

	if (reflect.IsReflectProperty()) {
		ReflectProperty(viewProjectionMatrix)[IShader::BindInput(IShader::BindInput::TRANSFORM_VIEWPROJECTION)];
		ReflectProperty(viewMatrix)[IShader::BindInput(IShader::BindInput::TRANSFORM_VIEW)];
		ReflectProperty(lightReprojectionMatrix)[IShader::BindInput(IShader::BindInput::GENERAL)];
		ReflectProperty(lightColor)[IShader::BindInput(IShader::BindInput::GENERAL)];
		ReflectProperty(lightPosition)[IShader::BindInput(IShader::BindInput::GENERAL)];
		ReflectProperty(invScreenSize)[IShader::BindInput(IShader::BindInput::GENERAL)];
		ReflectProperty(noiseTexture);
		ReflectProperty(lightDepthTexture);
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

PhaseComponentConfig::TaskData::~TaskData() {
	assert(warpData.empty());
}

void PhaseComponentConfig::InstanceGroup::Cleanup() {
	for (size_t i = 0; i < instancedData.size(); i++) {
		instancedData[i].Clear();
	}

	drawCallDescription = IRender::Resource::DrawCallDescription();
	drawCallResource = nullptr;
	instanceCount = 0;
}

void PhaseComponentConfig::TaskData::Cleanup(IRender& render) {
	OPTICK_EVENT();
	for (size_t i = 0; i < warpData.size(); i++) {
		WarpData& data = warpData[i];
		for (size_t k = 0; k < data.runtimeResources.size(); k++) {
			render.DeleteResource(renderQueue, data.runtimeResources[k]);
		}

		data.runtimeResources.clear();
		data.bytesCache.Reset();
		data.worldGlobalBufferMap.clear();
		data.dataUpdaters.clear();

		// to avoid frequently memory alloc/dealloc
		// data.instanceGroups.clear();
		for (WarpData::InstanceGroupMap::iterator ip = data.instanceGroups.begin(); ip != data.instanceGroups.end();) {
			InstanceGroup& group = (*ip).second;

#ifdef _DEBUG
			group.drawCallDescription.bufferResources.clear();
			group.drawCallDescription.textureResources.clear();
#endif

			if (group.instanceCount == 0) {
				// to be deleted.
				data.instanceGroups.erase(ip++);
			} else {
				group.Cleanup();
				++ip;
			}
		}
	}
}

void PhaseComponentConfig::TaskData::Destroy(IRender& render) {
	OPTICK_EVENT();
	Cleanup(render);
	warpData.clear();

	render.DeleteResource(renderQueue, renderTarget);
	render.DeleteQueue(renderQueue);
}

PhaseComponent::PhaseComponent(const TShared<RenderFlowComponent>& renderFlow, const String& portName) : hostEntity(nullptr), maxTracePerTick(8), renderQueue(nullptr), stateSetupResource(nullptr), statePostResource(nullptr), stateShadowResource(nullptr), range(32, 32, 32), resolution(512, 512), lightCollector(this), renderFlowComponent(std::move(renderFlow)), lightPhaseViewPortName(portName), rootEntity(nullptr) {}

PhaseComponent::~PhaseComponent() {}

void PhaseComponent::Initialize(Engine& engine, Entity* entity) {
	OPTICK_EVENT();
	if (rootEntity != entity) { // Set Host?
		Component::Initialize(engine, entity);
		assert(hostEntity == nullptr);
		assert(renderQueue == nullptr);
		hostEntity = entity;

		SnowyStream& snowyStream = engine.snowyStream;
		const String path = "[Runtime]/MeshResource/StandardQuad";
		meshResource = snowyStream.CreateReflectedResource(UniqueType<MeshResource>(), path, true, ResourceBase::RESOURCE_VIRTUAL);

		tracePipeline = snowyStream.CreateReflectedResource(UniqueType<ShaderResource>(), ShaderResource::GetShaderPathPrefix() + UniqueType<MultiHashTracePass>::Get()->GetBriefName(), true, ResourceBase::RESOURCE_VIRTUAL)->QueryInterface(UniqueType<ShaderResourceImpl<MultiHashTracePass> >());
		setupPipeline = snowyStream.CreateReflectedResource(UniqueType<ShaderResource>(), ShaderResource::GetShaderPathPrefix() + UniqueType<MultiHashSetupPass>::Get()->GetBriefName(), true, ResourceBase::RESOURCE_VIRTUAL)->QueryInterface(UniqueType<ShaderResourceImpl<MultiHashSetupPass> >());
		shadowPipeline = snowyStream.CreateReflectedResource(UniqueType<ShaderResource>(), ShaderResource::GetShaderPathPrefix() + UniqueType<ConstMapPass>::Get()->GetBriefName(), true, ResourceBase::RESOURCE_VIRTUAL)->QueryInterface(UniqueType<ShaderResourceImpl<ConstMapPass> >());

		IRender& render = engine.interfaces.render;
		IRender::Device* device = engine.snowyStream.GetRenderDevice();
		renderQueue = render.CreateQueue(device);

		statePostResource = render.CreateResource(device, IRender::Resource::RESOURCE_RENDERSTATE);
		IRender::Resource::RenderStateDescription state;
		state.cull = 1;
		state.fill = 1;
		state.blend = 0;
		state.colorWrite = 1;
		state.depthTest = IRender::Resource::RenderStateDescription::DISABLED;
		state.depthWrite = 0;
		state.stencilTest = IRender::Resource::RenderStateDescription::DISABLED;
		state.stencilWrite = 0;
		render.UploadResource(renderQueue, statePostResource, &state);

		stateSetupResource = render.CreateResource(device, IRender::Resource::RESOURCE_RENDERSTATE);
		state.cull = 1;
		state.fill = 1;
		state.blend = 0;
		state.colorWrite = 1;
		state.depthTest = IRender::Resource::RenderStateDescription::GREATER_EQUAL;
		state.depthWrite = 1;
		state.stencilTest = IRender::Resource::RenderStateDescription::DISABLED;
		state.stencilWrite = 0;
		render.UploadResource(renderQueue, stateSetupResource, &state);

		stateShadowResource = render.CreateResource(device, IRender::Resource::RESOURCE_RENDERSTATE);
		state.cull = 1;
		state.fill = 1;
		state.blend = 0;
		state.colorWrite = 0;
		state.depthTest = IRender::Resource::RenderStateDescription::GREATER_EQUAL;
		state.depthWrite = 1;
		state.stencilTest = IRender::Resource::RenderStateDescription::DISABLED;
		state.stencilWrite = 0;
		state.cullFrontFace = 0; // It's not the same with shadow map, we must not cull front face since some internal pixels may be incorrectly lighted.
		render.UploadResource(renderQueue, stateShadowResource, &state);
	}
}

Tiny::FLAG PhaseComponent::GetEntityFlagMask() const {
	return Entity::ENTITY_HAS_TICK_EVENT | Entity::ENTITY_HAS_SPECIAL_EVENT;
}

void PhaseComponent::Uninitialize(Engine& engine, Entity* entity) {
	OPTICK_EVENT();
	if (rootEntity == entity) {
		rootEntity = nullptr;
	} else {
		IRender::Queue* queue = reinterpret_cast<IRender::Queue*>(tracePipeline->GetResourceManager().GetContext());
		IRender& render = engine.interfaces.render;
		for (size_t j = 0; j < tasks.size(); j++) {
			TaskData& task = tasks[j];
			task.Destroy(render);
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

		render.DeleteResource(queue, stateSetupResource);
		render.DeleteResource(queue, statePostResource);
		render.DeleteResource(queue, stateShadowResource);
		render.DeleteQueue(renderQueue);

		stateSetupResource = statePostResource = stateShadowResource = nullptr;
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
	OPTICK_EVENT();
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

		phase.depth->description.state.immutable = false;
		phase.depth->description.state.attachment = true;
		phase.depth->description.dimension = UShort3(resolution.x(), resolution.y(), 1);
		phase.depth->description.state.format = IRender::Resource::TextureDescription::FLOAT;
		phase.depth->description.state.layout = IRender::Resource::TextureDescription::DEPTH;
		phase.depth->Flag().fetch_or(Tiny::TINY_MODIFIED, std::memory_order_release);
		phase.depth->GetResourceManager().InvokeUpload(phase.depth(), renderQueue);

		phase.irradiance = engine.snowyStream.CreateReflectedResource(UniqueType<TextureResource>(), ResourceBase::GenerateLocation("PhaseIrradiance", &phase), false, ResourceBase::RESOURCE_VIRTUAL);
		phase.irradiance->description.state.immutable = false;
		phase.irradiance->description.state.attachment = true;
		phase.irradiance->description.dimension = UShort3(resolution.x(), resolution.y(), 1);
		phase.irradiance->description.state.format = IRender::Resource::TextureDescription::HALF;
		phase.irradiance->description.state.layout = IRender::Resource::TextureDescription::RGB10PACK;
		phase.irradiance->Flag().fetch_or(Tiny::TINY_MODIFIED, std::memory_order_release);
		phase.irradiance->GetResourceManager().InvokeUpload(phase.irradiance(), renderQueue);

		phase.baseColorOcclusion = engine.snowyStream.CreateReflectedResource(UniqueType<TextureResource>(), ResourceBase::GenerateLocation("PhaseBaseColorOcclusion", &phase), false, ResourceBase::RESOURCE_VIRTUAL);
		phase.baseColorOcclusion->description.state.immutable = false;
		phase.baseColorOcclusion->description.state.attachment = true;
		phase.baseColorOcclusion->description.dimension = UShort3(resolution.x(), resolution.y(), 1);
		phase.baseColorOcclusion->description.state.format = IRender::Resource::TextureDescription::UNSIGNED_BYTE;
		phase.baseColorOcclusion->description.state.layout = IRender::Resource::TextureDescription::RGBA;
		phase.baseColorOcclusion->Flag().fetch_or(Tiny::TINY_MODIFIED, std::memory_order_release);
		phase.baseColorOcclusion->GetResourceManager().InvokeUpload(phase.baseColorOcclusion(), renderQueue);

		phase.normalRoughnessMetallic = engine.snowyStream.CreateReflectedResource(UniqueType<TextureResource>(), ResourceBase::GenerateLocation("PhaseNormalRoughness", &phase), false, ResourceBase::RESOURCE_VIRTUAL);
		phase.normalRoughnessMetallic->description.state.immutable = false;
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
	OPTICK_EVENT();
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
	OPTICK_EVENT();
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
			if (!debugPath.empty()) {
				for (size_t k = 0; k < task.textures.size(); k++) {
					render.RequestDownloadResource(task.renderQueue, task.textures[k]->GetRenderResource(), &task.textures[k]->description);
					status = TaskData::STATUS_DOWNLOADED;
				}
			}

			// engine.mythForest.StartCaptureFrame("cap", "");
			render.FlushQueue(task.renderQueue);
			bakeQueues.emplace_back(task.renderQueue);
			finalStatus.store(status, std::memory_order_release);
			// render.PresentQueues(&task.renderQueue, 1, IRender::PRESENT_EXECUTE_ALL);
			// engine.mythForest.EndCaptureFrame();
		} else if (task.status == TaskData::STATUS_DOWNLOADED) {
			for (size_t k = 0; k < task.textures.size(); k++) {
				render.CompleteDownloadResource(task.renderQueue, task.textures[k]->GetRenderResource());
			}

			bakeQueues.emplace_back(task.renderQueue);

			// Save data asynchronized
			uint32_t frameIndex = engine.GetFrameIndex();
			for (size_t j = 0; j < task.textures.size(); j++) {
				assert(!task.textures[j]->GetLocation().empty());
				engine.GetKernel().QueueRoutine(this, CreateTaskContextFree(Wrap(this, &PhaseComponent::CoTaskWriteDebugTexture), std::ref(engine), (uint32_t)safe_cast<uint32_t>(frameIndex * tasks.size() + i), std::move(task.textures[j]->description.data), task.textures[j]));
			}

			finalStatus.store(TaskData::STATUS_BAKED, std::memory_order_release);
		}
	}

	// Commit bakes
	if (!bakeQueues.empty()) {
		render.PresentQueues(&bakeQueues[0], safe_cast<uint32_t>(bakeQueues.size()), IRender::PRESENT_EXECUTE_ALL);
	}
}

static String ParseStageFromTexturePath(const String& path) {
	char stage[256] = "Unknown";
	const char* p = path.c_str() + 17;
	while (*p != '/') p++;

	return String(path.c_str() + 17, p);
}

void PhaseComponent::CoTaskWriteDebugTexture(Engine& engine, uint32_t index, Bytes& data, const TShared<TextureResource>& texture) {
	OPTICK_EVENT();
	if (!debugPath.empty()) {
		std::stringstream ss;
		ss << debugPath << "phase_" << index << "_" << ParseStageFromTexturePath(texture->GetLocation()) << ".png";
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
		} else {
			/*
			assert(layout == IRender::Resource::TextureDescription::RGBA);
			assert(format == IRender::Resource::TextureDescription::UNSIGNED_BYTE);
			size_t count = description.dimension.x() * description.dimension.y();
			uint8_t* target = (uint8_t*)data.GetData();
			for (size_t i = 0; i < count; i++) {
				std::swap(target[i * 4], target[i * 4 + 2]);
			}*/
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
	OPTICK_EVENT();
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

						for (size_t k = 0; k < group.instancedData.size(); k++) {
							Bytes& data = group.instancedData[k];
							assert(!data.Empty());
							if (!data.Empty()) {
								size_t viewSize = data.GetViewSize();
								IRender::Resource* buffer = render.CreateResource(render.GetQueueDevice(queue), IRender::Resource::RESOURCE_BUFFER);
								IRender::Resource::BufferDescription desc;
								desc.format = IRender::Resource::BufferDescription::FLOAT;
								desc.usage = IRender::Resource::BufferDescription::INSTANCED;
								desc.component = safe_cast<uint8_t>(viewSize / (group.instanceCount * sizeof(float)));
								desc.data.Resize(viewSize);
								desc.data.Import(0, data);
								render.UploadResource(queue, buffer, &desc);

								// assign instanced buffer	
								// assert(group.drawCallDescription.bufferResources[output.slot].buffer == nullptr);
								IRender::Resource::DrawCallDescription::BufferRange& bufferRange = group.drawCallDescription.bufferResources[k];
								bufferRange.buffer = buffer;
								warpData.runtimeResources.emplace_back(buffer);
							}
						}

						group.drawCallDescription.instanceCounts.x() = group.instanceCount;

						assert(PassBase::ValidateDrawCall(group.drawCallDescription));
						IRender::Resource* drawCall = render.CreateResource(render.GetQueueDevice(queue), IRender::Resource::RESOURCE_DRAWCALL);
						IRender::Resource::DrawCallDescription dc = group.drawCallDescription; // make copy
						render.UploadResource(queue, drawCall, &dc);
						render.ExecuteResource(queue, drawCall);

						warpData.runtimeResources.emplace_back(drawCall);
						group.Cleanup(); // for next reuse
					}
				}

				finalStatus.store(TaskData::STATUS_ASSEMBLED, std::memory_order_release);
			}
		}
	}
}

void PhaseComponent::TaskAssembleTaskBounce(Engine& engine, TaskData& task, const UpdatePointBounce& bakePoint) {
	OPTICK_EVENT();
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
	task.textures.clear();
	task.textures.emplace_back(toPhase.irradiance);

	// TODO: fill params
	MultiHashTraceFS& fs = static_cast<MultiHashTracePass&>(toPhase.tracePipeline->GetPass()).shaderMultiHashTrace;

	// changing state
	render.UploadResource(task.renderQueue, task.renderTarget, &desc);
	render.ExecuteResource(task.renderQueue, statePostResource);
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
	OPTICK_EVENT();

	IRender& render = engine.interfaces.render;
	assert(task.status == TaskData::STATUS_DISPATCHED);
	IRender::Resource::RenderTargetDescription desc;

	const Shadow& shadow = shadows[bakePoint.shadowIndex];
	desc.depthStorage.resource = shadow.shadow->GetRenderResource();
	desc.depthStorage.loadOp = IRender::Resource::RenderTargetDescription::CLEAR;
	desc.depthStorage.storeOp = IRender::Resource::RenderTargetDescription::DEFAULT;

	render.UploadResource(task.renderQueue, task.renderTarget, &desc);
	render.ExecuteResource(task.renderQueue, stateShadowResource);
	render.ExecuteResource(task.renderQueue, task.renderTarget);

	task.pipeline = shadowPipeline();
	task.textures.clear();
	task.textures.emplace_back(shadow.shadow);
	// task.texture = nullptr;
	Collect(engine, task, shadow.viewMatrix, shadow.viewMatrix * shadow.projectionMatrix);
}

void PhaseComponent::CompleteCollect(Engine& engine, TaskData& taskData) {}

void PhaseComponent::Collect(Engine& engine, TaskData& taskData, const MatrixFloat4x4& viewMatrix, const MatrixFloat4x4& worldMatrix) {
	OPTICK_EVENT();
	PerspectiveCamera camera;
	CaptureData captureData;
	camera.UpdateCaptureData(captureData, Math::QuickInverse(viewMatrix));
	assert(rootEntity->GetWarpIndex() == GetWarpIndex());
	WorldInstanceData instanceData;
	instanceData.worldMatrix = worldMatrix;
	instanceData.instancedColor.x() = 1.0f / resolution.x();
	instanceData.instancedColor.y() = 1.0f / resolution.y();
	std::atomic<uint32_t>& finalStatus = reinterpret_cast<std::atomic<uint32_t>&>(taskData.status);
	finalStatus.store(TaskData::STATUS_ASSEMBLING, std::memory_order_release);
	taskData.Cleanup(engine.interfaces.render);

	std::atomic_thread_fence(std::memory_order_acquire);
	CollectComponentsFromEntity(engine, taskData, instanceData, captureData, rootEntity);
}

void PhaseComponent::CoTaskAssembleTaskSetup(Engine& engine, TaskData& task, const UpdatePointSetup& bakePoint) {
	OPTICK_EVENT();

	IRender& render = engine.interfaces.render;
	assert(task.status == TaskData::STATUS_DISPATCHED);
	IRender::Resource::RenderTargetDescription desc;
	Phase& phase = phases[bakePoint.phaseIndex];

	TextureResource* rt[] = {
		phase.baseColorOcclusion(),
		phase.normalRoughnessMetallic()
	};

	for (size_t k = 0; k < sizeof(rt) / sizeof(rt[0]); k++) {
		IRender::Resource::RenderTargetDescription::Storage storage;
		storage.resource = rt[k]->GetRenderResource();
		storage.loadOp = IRender::Resource::RenderTargetDescription::CLEAR;
		storage.storeOp = IRender::Resource::RenderTargetDescription::DEFAULT;
		desc.colorStorages.emplace_back(storage);
	}

	desc.depthStorage.resource = phase.depth->GetRenderResource();
	desc.depthStorage.loadOp = IRender::Resource::RenderTargetDescription::CLEAR;
	desc.depthStorage.storeOp = IRender::Resource::RenderTargetDescription::DEFAULT;

	render.UploadResource(task.renderQueue, task.renderTarget, &desc);
	render.ExecuteResource(task.renderQueue, stateSetupResource);
	render.ExecuteResource(task.renderQueue, task.renderTarget);

	task.textures.clear();
	task.textures.emplace_back(phase.baseColorOcclusion);
	task.textures.emplace_back(phase.normalRoughnessMetallic);

	task.pipeline = setupPipeline();
	task.camera = phase.camera;
	task.worldGlobalData.noiseTexture.resource = phase.noiseTexture->GetRenderResource();
	task.worldGlobalData.viewMatrix = phase.viewMatrix;
	task.worldGlobalData.viewProjectionMatrix = phase.viewMatrix * phase.projectionMatrix;

	// shadow
	phase.shadowIndex = bakePoint.shadowIndex;
	Shadow& shadow = shadows[phase.shadowIndex];
	const UShort3& dim = shadow.shadow->description.dimension;
	task.worldGlobalData.lightReprojectionMatrix = Math::InverseProjection(phase.projectionMatrix) * Math::QuickInverse(phase.viewMatrix) * shadow.viewMatrix * shadow.projectionMatrix;
	task.worldGlobalData.lightColor = shadow.lightElement.colorAttenuation;
	task.worldGlobalData.lightPosition = shadow.lightElement.position;
	task.worldGlobalData.invScreenSize = Float2(1.0f / dim.x(), 1.0f / dim.y());
	task.worldGlobalData.lightDepthTexture.resource = shadow.shadow->GetRenderResource();

	Collect(engine, task, phase.viewMatrix, MatrixFloat4x4::Identity());
}

void PhaseComponent::DispatchTasks(Engine& engine) {
	size_t n = 0;
	Kernel& kernel = engine.GetKernel();
	ThreadPool& threadPool = kernel.GetThreadPool();

	for (size_t i = 0; i < tasks.size(); i++) {
		TaskData& task = tasks[n];
		std::atomic<uint32_t>& finalStatus = reinterpret_cast<std::atomic<uint32_t>&>(task.status);
		if (task.status == TaskData::STATUS_START) {
			if (!bakePointShadows.empty()) {
				finalStatus.store(TaskData::STATUS_DISPATCHED, std::memory_order_relaxed);
				const UpdatePointShadow& shadow = bakePointShadows.top();
				threadPool.Push(CreateCoTaskContextFree(kernel, Wrap(this, &PhaseComponent::CoTaskAssembleTaskShadow), std::ref(engine), std::ref(task), shadow));
				bakePointShadows.pop();
			} else if (!bakePointSetups.empty()) {
				finalStatus.store(TaskData::STATUS_DISPATCHED, std::memory_order_relaxed);
				const UpdatePointSetup& setup = bakePointSetups.top();
				threadPool.Push(CreateCoTaskContextFree(kernel, Wrap(this, &PhaseComponent::CoTaskAssembleTaskSetup), std::ref(engine), std::ref(task), setup));
				bakePointSetups.pop();
			} else if (!bakePointBounces.empty()) {
				finalStatus.store(TaskData::STATUS_DISPATCHED, std::memory_order_relaxed);
				const UpdatePointBounce& bounce = bakePointBounces.top();
				TaskAssembleTaskBounce(engine, task, bounce);
				bakePointBounces.pop();
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
							ip->second.buffers[i].buffer = res;
							warpData.runtimeResources.emplace_back(res);
						}
					}

					if (!textureResources.empty()) {
						ip->second.textures.resize(updater.GetTextureCount());
						for (size_t j = 0; j < textureResources.size(); j++) {
							IRender::Resource* res = textureResources[j];
							if (res != nullptr) {
								ip->second.textures[j] = res;
							}
						}
					}

					for (size_t k = 0; k < bufferResources.size(); k++) {
						IRender::Resource::DrawCallDescription::BufferRange& range = bufferResources[k];
						if (range.buffer != nullptr) {
							ip->second.buffers[k] = range;
						}
					}
				}

				if (!ip->second.textures.empty()) {
					for (size_t m = 0; m < group.drawCallDescription.textureResources.size(); m++) {
						IRender::Resource*& texture = group.drawCallDescription.textureResources[m];
						if (ip->second.textures[m] != nullptr) {
							texture = ip->second.textures[m];
						}
					}
				}

				for (size_t n = 0; n < group.drawCallDescription.bufferResources.size(); n++) {
					IRender::Resource::DrawCallDescription::BufferRange& bufferRange = group.drawCallDescription.bufferResources[n];
					if (ip->second.buffers[n].buffer != nullptr) {
						bufferRange = ip->second.buffers[n];
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
	OPTICK_EVENT();
	MatrixFloat4x4 projectionMatrix = Math::MatrixOrtho(range);
	bakePointShadows = std::stack<UpdatePointShadow>();
	shadows.resize(elements.size());

	// generate shadow tasks
	for (size_t i = 0; i < elements.size(); i++) {
		Shadow& shadow = shadows[i];
		const LightElement& lightElement = elements[i];
		if (!shadow.shadow) {
			shadow.shadow = engine.snowyStream.CreateReflectedResource(UniqueType<TextureResource>(), ResourceBase::GenerateLocation("PhaseShadow", &shadow), false, ResourceBase::RESOURCE_VIRTUAL);
			shadow.shadow->description.state.immutable = false;
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

		shadow.viewMatrix = Math::MatrixLookAt(view, dir, up);
		shadow.projectionMatrix = projectionMatrix;

		UpdatePointShadow bakePointShadow;
		bakePointShadow.shadowIndex = safe_cast<uint32_t>(i);
		bakePointShadows.push(bakePointShadow);
	}

	// generate setup tasks
	bakePointSetups = std::stack<UpdatePointSetup>();
	if (!elements.empty()) {
		for (size_t j = 0; j < phases.size(); j++) {
			UpdatePointSetup bakePoint;
			bakePoint.phaseIndex = safe_cast<uint32_t>(j);
			bakePoint.shadowIndex = safe_cast<uint32_t>(rand() % elements.size());
			bakePointSetups.push(bakePoint);
		}
	}

	// stop all bounces
	// bakePointBounces = std::stack<UpdatePointBounce>();
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
	instanceData.boundingBox = inst.boundingBox;

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
	OPTICK_EVENT();
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
	worldInstance.boundingBox = Float3Pair(Float3(FLT_MAX, FLT_MAX, FLT_MAX), Float3(-FLT_MAX, -FLT_MAX, -FLT_MAX));
	LightConfig::CaptureData captureData;

	std::atomic_thread_fence(std::memory_order_acquire);
	CollectComponentsFromEntity(engine, *taskData, worldInstance, captureData, entity);
}

void PhaseComponent::Update(Engine& engine, const Float3& center) {
	OPTICK_EVENT();
	// generate bake points
	srand(0);
	bakePointSetups = std::stack<UpdatePointSetup>();

	PerspectiveCamera camera;
	camera.nearPlane = 0.1f;
	camera.farPlane = 100.0f;
	camera.aspect = 1.0f;
	camera.fov = PI / 2;

	MatrixFloat4x4 projectionMatrix = Math::MatrixPerspective(camera.fov, camera.aspect, camera.nearPlane, camera.farPlane);

	// Adjust phases?
	for (size_t i = 0; i < phases.size(); i++) {
		float theta = RandFloat() * 2 * PI;
		float phi = RandFloat() * PI;
		Float3 view = range * Float3(cos(theta), sin(theta), cos(phi));
		Float3 dir = -view;
		Float3 up(RandFloat(), RandFloat(), RandFloat());

		phases[i].camera = camera;
		phases[i].projectionMatrix = projectionMatrix;
		phases[i].viewMatrix = Math::MatrixLookAt(view + center, dir, up);
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
		instanceData.instancedColor = inst.instancedColor;

		uint32_t currentWarpIndex = engine.GetKernel().GetCurrentWarpIndex();
		WarpData& warpData = task.warpData[currentWarpIndex == ~(uint32_t)0 ? GetWarpIndex() : currentWarpIndex];
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

			if (component->GetEntityFlagMask() & Entity::ENTITY_HAS_RENDERABLE) {
				if (transformComponent != nullptr) {
					RenderableComponent* renderableComponent = static_cast<RenderableComponent*>(component);
					if (!(renderableComponent->Flag().load(std::memory_order_relaxed) & RenderableComponent::RENDERABLECOMPONENT_CAMERAVIEW)) {
						// generate instance color
						instanceData.instancedColor.z() = RandFloat();
						instanceData.instancedColor.w() = RandFloat();
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
	OPTICK_EVENT();
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

