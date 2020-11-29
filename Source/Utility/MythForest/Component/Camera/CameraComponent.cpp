#include "CameraComponent.h"
#include "../../../../Core/Interface/IMemory.h"
#include "../Animation/AnimationComponent.h"
#include "../Batch/BatchComponent.h"
#include "../Space/SpaceComponent.h"
#include "../Transform/TransformComponent.h"
#include "../Explorer/ExplorerComponent.h"
#include "../EnvCube/EnvCubeComponent.h"
#include "../Light/LightComponent.h"
#include "../Visibility/VisibilityComponent.h"
#include "../Phase/PhaseComponent.h"
#include "../Renderable/RenderableComponent.h"
#include "../RenderFlow/RenderFlowComponent.h"
#include "../RenderFlow/RenderPort/RenderPortRenderTarget.h"
#include "../RenderFlow/RenderPort/RenderPortLightSource.h"
#include "../RenderFlow/RenderPort/RenderPortCameraView.h"
#include "../RenderFlow/RenderPort/RenderPortCommandQueue.h"
#include "../../Engine.h"
#include "../../../BridgeSunset/BridgeSunset.h"
#include "../../../SnowyStream/SnowyStream.h"
#include <cmath>
#include <iterator>
#include <utility>

using namespace PaintsNow;

// From glu source code
const double PI = 3.14159265358979323846;
enum {
	STENCIL_SHADOW = 0x10,
	// STENCIL_REFLECTION = 0x20,
	// STENCIL_BLOOM = 0x40,
	STENCIL_LIGHTING = 0x80
};

CameraComponent::TaskData::WarpData::WarpData() : entityCount(0), visibleEntityCount(0), triangleCount(0) {}

CameraComponent::CameraComponent(const TShared<RenderFlowComponent>& prenderFlowComponent, const String& name)
: collectedEntityCount(0), collectedVisibleEntityCount(0), collectedTriangleCount(0), viewDistance(256), rootEntity(nullptr), jitterIndex(0), renderFlowComponent(std::move(prenderFlowComponent)), cameraViewPortName(name) {
	Flag().fetch_or(CAMERACOMPONENT_PERSPECTIVE | CAMERACOMPONENT_UPDATE_COMMITTED, std::memory_order_relaxed);
}

void CameraComponent::UpdateJitterMatrices(CameraComponentConfig::WorldGlobalData& worldGlobalData) {
	if (Flag().load(std::memory_order_relaxed) & CAMERACOMPONENT_SUBPIXEL_JITTER) {
		jitterIndex = (jitterIndex + 1) % 9;

		MatrixFloat4x4 jitterMatrix = MatrixFloat4x4::Identity();
		UShort2 resolution = renderFlowComponent->GetMainResolution();
		static float jitterX[9] = { 1.0f / 2.0f, 1.0f / 4.0f, 3.0f / 4.0f, 1.0f / 8.0f, 5.0f / 8.0f, 3.0f / 8.0f, 7.0f / 8.0f, 1.0f / 16.0f, 9.0f / 16.0f };
		static float jitterY[9] = { 1.0f / 3.0f, 2.0f / 3.0f, 1.0f / 9.0f, 4.0f / 9.0f, 7.0f / 9.0f, 2.0f / 9.0f, 5.0f / 9.0f, 8.0f / 9.0f, 1.0f / 27.0f };
		Float2 jitterOffset((jitterX[jitterIndex] - 0.5f) / Math::Max((int)resolution.x(), 1), (jitterY[jitterIndex] - 0.5f) / Math::Max((int)resolution.y(), 1));
		// jitterOffset *= 0.25f;
		jitterMatrix(3, 0) += jitterOffset.x();
		jitterMatrix(3, 1) += jitterOffset.y();
		jitterIndex = (jitterIndex + 1) % 9;

		worldGlobalData.jitterMatrix = jitterMatrix;
		worldGlobalData.jitterOffset = jitterOffset;
	}
}

void CameraComponent::UpdateRootMatrices(const MatrixFloat4x4& cameraWorldMatrix) {
	MatrixFloat4x4 projectionMatrix = (Flag().load(std::memory_order_relaxed) & CAMERACOMPONENT_PERSPECTIVE) ? Math::Perspective(fov, aspect, nearPlane, farPlane) : Math::Ortho(Float3(1, 1, 1));

	MatrixFloat4x4 viewMatrix = Math::QuickInverse(cameraWorldMatrix);
	nextTaskData->worldGlobalData.inverseViewMatrix = cameraWorldMatrix;
	nextTaskData->worldGlobalData.projectionMatrix = projectionMatrix;
	nextTaskData->worldGlobalData.viewMatrix = viewMatrix;
	nextTaskData->worldGlobalData.viewProjectionMatrix = viewMatrix * projectionMatrix;
	nextTaskData->worldGlobalData.tanHalfFov = (float)tan(fov / 2.0f);
	nextTaskData->worldGlobalData.viewPosition = Float3(cameraWorldMatrix(3, 0), cameraWorldMatrix(3, 1), cameraWorldMatrix(3, 2));
	nextTaskData->worldGlobalData.lastViewProjectionMatrix = prevTaskData->worldGlobalData.viewProjectionMatrix;

	Flag().fetch_and(~TINY_MODIFIED, std::memory_order_release);

}

RenderFlowComponent* CameraComponent::GetRenderFlowComponent() const {
	return renderFlowComponent();
}

Tiny::FLAG CameraComponent::GetEntityFlagMask() const {
	return Entity::ENTITY_HAS_TICK_EVENT;
}

void CameraComponent::Initialize(Engine& engine, Entity* entity) {
	if (rootEntity != entity) { // Set Host?
		BaseClass::Initialize(engine, entity);
		RenderPort* port = renderFlowComponent->BeginPort(cameraViewPortName);
		if (port != nullptr) {
			RenderPortCameraView* cameraViewPort = port->QueryInterface(UniqueType<RenderPortCameraView>());
			if (cameraViewPort != nullptr) {
				cameraViewPort->eventTickHooks += Wrap(this, &CameraComponent::OnTickCameraViewPort);
			}

			renderFlowComponent->EndPort(port);
		}

		IRender::Device* device = engine.snowyStream.GetRenderDevice();
		uint32_t warpCount = engine.bridgeSunset.GetKernel().GetWarpCount();
		prevTaskData = TShared<TaskData>::From(new TaskData(warpCount));
		nextTaskData = TShared<TaskData>::From(new TaskData(warpCount));
	}
}

void CameraComponent::Uninitialize(Engine& engine, Entity* entity) {
	if (rootEntity == entity) {
		rootEntity = nullptr;
	} else {
		if (Flag().load(std::memory_order_acquire) & CAMERACOMPONENT_UPDATE_COLLECTING) {
			Kernel& kernel = engine.GetKernel();
			ThreadPool& threadPool = kernel.GetThreadPool();
			if (threadPool.GetThreadCount() != 0) {
				Wait(threadPool, CAMERACOMPONENT_UPDATE_COLLECTING, 0);
			}
		}

		RenderPort* port = renderFlowComponent->BeginPort(cameraViewPortName);
		if (port != nullptr) {
			RenderPortCameraView* cameraViewPort = port->QueryInterface(UniqueType<RenderPortCameraView>());
			if (cameraViewPort != nullptr) {
				cameraViewPort->eventTickHooks -= Wrap(this, &CameraComponent::OnTickCameraViewPort);
			}

			renderFlowComponent->EndPort(port);
		}

		IRender& render = engine.interfaces.render;
		prevTaskData->Destroy(render);
		prevTaskData = nullptr;
		nextTaskData->Destroy(render);
		nextTaskData = nullptr;

		BaseClass::Uninitialize(engine, entity);
	}
}

// Event Dispatcher
void CameraComponent::DispatchEvent(Event& event, Entity* entity) {
	if (event.eventID == Event::EVENT_TICK && entity != rootEntity) {
		OnTickHost(event.engine, entity);
	}
}

void CameraComponent::Instancing(Engine& engine, TaskData& taskData) {
	// instancing
	// do not sort at this moment.
	IRender& render = engine.interfaces.render;
	uint32_t entityCount = 0;
	uint32_t visibleEntityCount = 0;
	uint32_t triangleCount = 0;

	for (size_t j = 0; j < taskData.warpData.size(); j++) {
		TaskData::WarpData& warpData = taskData.warpData[j];
		TaskData::WarpData::InstanceGroupMap& instanceGroup = warpData.instanceGroups;
		entityCount += warpData.entityCount;
		visibleEntityCount += warpData.visibleEntityCount;
		triangleCount += warpData.triangleCount;

		for (TaskData::WarpData::InstanceGroupMap::iterator it = instanceGroup.begin(); it != instanceGroup.end(); ++it) {
			InstanceGroup& group = (*it).second;
			if (group.instanceCount == 0) continue;
			std::vector<std::key_value<RenderPolicy*, TaskData::PolicyData> >::iterator ip = std::binary_find(warpData.renderPolicyMap.begin(), warpData.renderPolicyMap.end(), group.renderPolicy());
			if (ip == warpData.renderPolicyMap.end()) {
				ip = std::binary_insert(warpData.renderPolicyMap, group.renderPolicy());
			}
		
			TaskData::PolicyData& policyData = ip->second;
			IRender::Queue* queue = policyData.portQueue;
			assert(queue != nullptr);

			for (size_t k = 0; k < group.instancedData.size(); k++) {
				Bytes& data = group.instancedData[k];
				if (!data.Empty()) {
					// assign instanced buffer	
					assert(group.drawCallDescription.bufferResources[k].buffer == nullptr);
					IRender::Resource::DrawCallDescription::BufferRange& bufferRange = group.drawCallDescription.bufferResources[k];
					bufferRange.buffer = policyData.instanceBuffer;
					bufferRange.offset = safe_cast<uint32_t>(policyData.instanceData.GetSize()); // TODO: alignment
					bufferRange.component = safe_cast<uint16_t>(data.GetSize() / (group.instanceCount * sizeof(float)));
					policyData.instanceData.Append(data);
				}
			}

			group.drawCallDescription.instanceCounts.x() = group.instanceCount;

			assert(PassBase::ValidateDrawCall(group.drawCallDescription));
			IRender::Resource*& drawCall = group.drawCallResource;
			if (drawCall == nullptr) {
				drawCall = render.CreateResource(render.GetQueueDevice(queue), IRender::Resource::RESOURCE_DRAWCALL);
				policyData.runtimeResources.emplace_back(drawCall);
			}

			assert(group.drawCallDescription.shaderResource != nullptr);
			render.UploadResource(queue, drawCall, &group.drawCallDescription);
		}

		for (size_t n = 0; n < warpData.renderPolicyMap.size(); n++) {
			TaskData::PolicyData& policyData = warpData.renderPolicyMap[n].second;
			if (!policyData.instanceData.Empty()) {
				IRender::Resource::BufferDescription desc;
				desc.data = std::move(policyData.instanceData);
				desc.format = IRender::Resource::BufferDescription::FLOAT;
				desc.usage = IRender::Resource::BufferDescription::INSTANCED;
				desc.component = 0; // will be overridden by drawcall.
				render.UploadResource(policyData.portQueue, policyData.instanceBuffer, &desc);
				policyData.instanceData.Clear();
			}
		}
	}

	collectedEntityCount = entityCount;
	collectedVisibleEntityCount = visibleEntityCount;
	collectedTriangleCount = triangleCount;
}

MatrixFloat4x4 CameraComponent::ComputeSmoothTrackTransform() const {
	MatrixFloat4x4 transform;
	currentState.rotation.WriteMatrix(transform);
	const Float3& scale = currentState.scale;
	const Float3& translation = currentState.translation;

	float s[16] = {
		scale.x(), 0, 0, 0,
		0, scale.y(), 0, 0,
		0, 0, scale.z(), 0,
		translation.x(), translation.y(), translation.z(), 1
	};

	return transform * MatrixFloat4x4(s);
}

void CameraComponent::UpdateTaskData(Engine& engine, Entity* hostEntity) {
	// Next collection ready? 
	if (!(Flag().fetch_and(~CAMERACOMPONENT_UPDATE_COMMITTED, std::memory_order_relaxed) & CAMERACOMPONENT_UPDATE_COMMITTED)) {
		return;
	}

	Flag().fetch_or(CAMERACOMPONENT_UPDATE_COLLECTING, std::memory_order_acq_rel);

	std::swap(prevTaskData, nextTaskData);
	// Tick new collection
	nextTaskData->Cleanup(engine.interfaces.render);

	std::atomic_thread_fence(std::memory_order_acquire);

	WorldInstanceData worldInstanceData;
	worldInstanceData.worldMatrix = MatrixFloat4x4::Identity();

	CaptureData captureData;
	TransformComponent* transformComponent = hostEntity->GetUniqueComponent(UniqueType<TransformComponent>());
	MatrixFloat4x4 localTransform = MatrixFloat4x4::Identity();
	if (transformComponent != nullptr) {
		// set smooth track
		if (Flag().load(std::memory_order_relaxed) & CAMERACOMPONENT_SMOOTH_TRACK) {
			currentState = targetState;
			targetState.rotation = transformComponent->GetRotationQuaternion();
			targetState.translation = transformComponent->GetTranslation();
			targetState.scale = transformComponent->GetScale();
		}

		localTransform = transformComponent->GetTransform();
	}

	UpdateRootMatrices(localTransform);
	UpdateCaptureData(captureData, localTransform);

	// Must called from entity thread
	assert(engine.GetKernel().GetCurrentWarpIndex() == rootEntity->GetWarpIndex());
	VisibilityComponent* visibilityComponent = rootEntity->GetUniqueComponent(UniqueType<VisibilityComponent>());
	const Bytes& visData = visibilityComponent != nullptr ? visibilityComponent->QuerySample(engine, Float3(localTransform(3, 0), localTransform(3, 1), localTransform(3, 2))) : Bytes::Null();

	captureData.visData = visData;
	CollectComponentsFromEntity(engine, *nextTaskData, worldInstanceData, captureData, rootEntity);
}

template <class T>
T* QueryPort(IRender& render, std::map<RenderPolicy*, RenderStage::Port*>& policyPortMap, RenderFlowComponent* renderFlowComponent, RenderPolicy* lastRenderPolicy, UniqueType<T>) {
	std::map<RenderPolicy*, RenderStage::Port*>::iterator it = policyPortMap.find(lastRenderPolicy);
	if (it != policyPortMap.end()) return it->second->QueryInterface(UniqueType<T>());

	RenderStage::Port*& port = policyPortMap[lastRenderPolicy];
	port = renderFlowComponent->BeginPort(lastRenderPolicy->renderPortName);

	if (port != nullptr) {
		T* t = port->QueryInterface(UniqueType<T>());
		if (t != nullptr) {
			t->BeginFrame(render);
			return t;
		}
	}

	return nullptr;
}

void CameraComponent::CommitRenderRequests(Engine& engine, TaskData& taskData, IRender::Queue* queue) {
	// commit to RenderFlowComponent
	// update data updaters
	if (renderFlowComponent) {
		IRender& render = engine.interfaces.render;
		uint32_t updateCount = 0;
		std::vector<TaskData::WarpData>& warpData = taskData.warpData;
		for (size_t i = 0; i < warpData.size(); i++) {
			TaskData::WarpData& w = warpData[i];
			for (size_t k = 0; k < w.dataUpdaters.size(); k++) {
				updateCount += w.dataUpdaters[k]->Update(render, queue);
			}
		}
		
		std::map<RenderPolicy*, RenderStage::Port*> policyPortMap;
		const Float3& viewPosition = taskData.worldGlobalData.viewPosition;

		TShared<TextureResource> cubeMapTexture;
		TShared<TextureResource> skyMapTexture;
		float minDist = FLT_MAX;
		for (size_t j = 0; j < taskData.warpData.size(); j++) {
			TaskData::WarpData& warpData = taskData.warpData[j];
			RenderPolicy* lastRenderPolicy = nullptr;
			RenderPortCommandQueue* lastCommandQueue = nullptr;
			IRender::Resource* lastRenderState = nullptr;

			// warpData.resourceQueue
			TaskData::WarpData::InstanceGroupMap& instanceGroups = warpData.instanceGroups;
			for (TaskData::WarpData::InstanceGroupMap::iterator it = instanceGroups.begin(); it != instanceGroups.end(); ++it) {
				InstanceGroup& group = (*it).second;
				if (group.instanceCount == 0) continue;

				if (group.renderPolicy != lastRenderPolicy) {
					lastRenderPolicy = group.renderPolicy();
					lastRenderState = nullptr;
					lastCommandQueue = lastRenderPolicy == nullptr ? nullptr : QueryPort(render, policyPortMap, renderFlowComponent(), lastRenderPolicy, UniqueType<RenderPortCommandQueue>());
					if (lastCommandQueue != nullptr) {
						std::vector<std::key_value<RenderPolicy*, TaskData::PolicyData> >::iterator ip = std::binary_find(warpData.renderPolicyMap.begin(), warpData.renderPolicyMap.end(), lastRenderPolicy);
						if (ip != warpData.renderPolicyMap.end()) {
							TaskData::PolicyData& policyData = ip->second;
							assert(policyData.instanceBuffer != nullptr);
							policyData.instanceBuffer = (IRender::Resource*)((size_t)policyData.instanceBuffer | 1);
							lastCommandQueue->MergeQueue(render, policyData.portQueue);
						}
					}
				}

				IRender::Resource* drawCallResource = (*it).second.drawCallResource;
				if (lastCommandQueue != nullptr && drawCallResource != nullptr) {
					IRender::Resource* renderState = group.renderStateResource;
					if (renderState != lastRenderState) {
						lastCommandQueue->CheckinState(render, renderState);
						lastRenderState = renderState;
					}

					lastCommandQueue->DrawElement(render, drawCallResource);
				}
			}

			size_t k = 0;
			for (size_t n = 0; n < warpData.renderPolicyMap.size(); n++) {
				TaskData::PolicyData& policyData = warpData.renderPolicyMap[n].second;
				IRender::Queue* queue = policyData.portQueue;
				bool cleanup = ((size_t)policyData.instanceBuffer & 1) == 0;
				policyData.instanceBuffer = (IRender::Resource*)((size_t)policyData.instanceBuffer & ~(size_t)1);

				if (cleanup) { // not visited, cleanup
					assert(policyData.instanceData.Empty());
					render.DeleteResource(queue, policyData.instanceBuffer);
					for (size_t m = 0; m < policyData.runtimeResources.size(); m++) {
						render.DeleteResource(queue, policyData.runtimeResources[m]);
					}

					RenderPortCommandQueue* port = QueryPort(render, policyPortMap, renderFlowComponent(), warpData.renderPolicyMap[n].first, UniqueType<RenderPortCommandQueue>());
					if (port == nullptr) {
						render.DeleteQueue(queue);
					} else {
						port->DeleteMergedQueue(render, queue);
					}

					policyData.runtimeResources.clear();
				} else if (k++ != n) {
					warpData.renderPolicyMap[k] = warpData.renderPolicyMap[n];
				}
			}

			warpData.renderPolicyMap.resize(k);

			// pass lights
			std::vector<std::pair<TShared<RenderPolicy>, LightElement> >& lightElements = warpData.lightElements;
			for (size_t u = 0; u < lightElements.size(); u++) {
				std::pair<TShared<RenderPolicy>, LightElement>& e = lightElements[u];
				LightElement& element = e.second;
				RenderPolicy* renderPolicy = e.first();
				if (renderPolicy != nullptr) {
					RenderPortLightSource* portLightSource = QueryPort(render, policyPortMap, renderFlowComponent(), renderPolicy, UniqueType<RenderPortLightSource>());
					if (portLightSource != nullptr) {
						if (portLightSource->lightElements.empty()) {
							portLightSource->stencilMask = STENCIL_LIGHTING;
						}

						portLightSource->stencilShadow = STENCIL_SHADOW;
						portLightSource->lightElements.emplace_back(std::move(element));

						if (element.position.w() == 0) {
							for (size_t i = 0; i < portLightSource->lightElements.size() - 1; i++) {
								LightElement& elem = portLightSource->lightElements[i];
								if (elem.position.w() != 0) {
									std::swap(portLightSource->lightElements.back(), elem);
									break;
								}
							}
						}
					}
				}
			}

			for (size_t j = 0; j < warpData.envCubeElements.size(); j++) {
				std::pair<TShared<RenderPolicy>, EnvCubeElement>& e = warpData.envCubeElements[j];
				EnvCubeElement& element = e.second;
				RenderPolicy* renderPolicy = e.first();
				if (renderPolicy != nullptr) {
					RenderPortLightSource* portLightSource = QueryPort(render, policyPortMap, renderFlowComponent(), renderPolicy, UniqueType<RenderPortLightSource>());
					float dist = Math::SquareLength(element.position - viewPosition);
					if (dist < minDist) {
						portLightSource->cubeMapTexture = element.cubeMapTexture ? element.cubeMapTexture : portLightSource->cubeMapTexture;
						portLightSource->skyMapTexture = element.skyMapTexture ? element.skyMapTexture : portLightSource->skyMapTexture;
						portLightSource->cubeStrength = element.cubeStrength;
						minDist = dist;
					}
				}
			}
		}

		for (std::map<RenderPolicy*, RenderPort*>::iterator it = policyPortMap.begin(); it != policyPortMap.end(); ++it) {
			it->second->EndFrame(render);
			renderFlowComponent->EndPort(it->second);
		}
	}
}

void CameraComponent::OnTickCameraViewPort(Engine& engine, RenderPort& renderPort, IRender::Queue* queue) {
	std::atomic_thread_fence(std::memory_order_acquire);

	TShared<TaskData> taskData;
	if ((Flag().load(std::memory_order_relaxed) & CAMERACOMPONENT_UPDATE_COLLECTED)) {
		taskData = nextTaskData;
		CommitRenderRequests(engine, *taskData, queue);
		Flag().fetch_and(~CAMERACOMPONENT_UPDATE_COLLECTED, std::memory_order_relaxed);
		Flag().fetch_or(CAMERACOMPONENT_UPDATE_COMMITTED, std::memory_order_acq_rel);
	} else {
		taskData = prevTaskData;
	}

	// Update jitter
	CameraComponentConfig::WorldGlobalData& worldGlobalData = taskData->worldGlobalData;
	std::vector<TaskData::WarpData>& warpData = taskData->warpData;
	IRender& render = engine.interfaces.render;

	if (Flag().load(std::memory_order_relaxed) & CAMERACOMPONENT_SMOOTH_TRACK) {
		const float ratio = 0.75f;
		Quaternion<float>::Interpolate(currentState.rotation, currentState.rotation, targetState.rotation, ratio);
		currentState.translation = Math::Interpolate(currentState.translation, targetState.translation, ratio);
		currentState.scale = Math::Interpolate(currentState.scale, targetState.scale, ratio);

		// recompute global matrices
		worldGlobalData.viewMatrix = Math::QuickInverse(ComputeSmoothTrackTransform());
	}
	
	worldGlobalData.viewProjectionMatrix = worldGlobalData.viewMatrix * worldGlobalData.projectionMatrix;

	// update camera view settings
	if (renderFlowComponent) {
		// update buffers
		if (Flag().load(std::memory_order_relaxed) & (CAMERACOMPONENT_SMOOTH_TRACK | CAMERACOMPONENT_SUBPIXEL_JITTER)) {
			for (size_t i = 0; i < warpData.size(); i++) {
				TaskData::WarpData& w = warpData[i];
				typedef std::vector<std::key_value<ShaderResource*, TaskData::WarpData::GlobalBufferItem> > GlobalMap;
				GlobalMap& globalMap = w.worldGlobalBufferMap;
				for (GlobalMap::iterator it = globalMap.begin(); it != globalMap.end(); ++it) {
					std::vector<Bytes> buffers;
					std::vector<IRender::Resource*> textureResources;
					std::vector<IRender::Resource::DrawCallDescription::BufferRange> bufferResources;
					it->second.globalUpdater.Snapshot(buffers, bufferResources, textureResources, worldGlobalData);

					assert(bufferResources.empty());
					assert(textureResources.empty());
					for (size_t i = 0; i < buffers.size(); i++) {
						Bytes& data = buffers[i];
						if (!data.Empty()) {
							IRender::Resource::BufferDescription desc;
							desc.usage = IRender::Resource::BufferDescription::UNIFORM;
							desc.component = 4;
							desc.dynamic = 1;
							desc.format = IRender::Resource::BufferDescription::FLOAT;
							desc.data = std::move(data);
							render.UploadResource(queue, it->second.buffers[i], &desc);
						}
					}
				}
			}
		}

		// CameraView settings
		RenderStage::Port* port = renderFlowComponent->BeginPort(cameraViewPortName);
		if (port != nullptr) {
			RenderPortCameraView* portCameraView = port->QueryInterface(UniqueType<RenderPortCameraView>());
			if (portCameraView != nullptr) {
				UpdateJitterMatrices(worldGlobalData);

				portCameraView->viewMatrix = worldGlobalData.viewMatrix;
				portCameraView->inverseViewMatrix = Math::QuickInverse(portCameraView->viewMatrix);
				portCameraView->projectionMatrix = worldGlobalData.projectionMatrix * worldGlobalData.jitterMatrix;
				portCameraView->inverseProjectionMatrix = Math::InverseProjection(portCameraView->projectionMatrix); // it's jittered
				portCameraView->reprojectionMatrix = portCameraView->inverseProjectionMatrix * portCameraView->inverseViewMatrix * worldGlobalData.lastViewProjectionMatrix;
				portCameraView->jitterOffset = worldGlobalData.jitterOffset;
			}

			renderFlowComponent->EndPort(port);
		}
	}

	worldGlobalData.lastViewProjectionMatrix = worldGlobalData.viewProjectionMatrix;
	worldGlobalData.viewProjectionMatrix = worldGlobalData.viewMatrix * worldGlobalData.projectionMatrix;

	renderPort.Flag().fetch_or(TINY_MODIFIED, std::memory_order_acq_rel);
}

void CameraComponent::OnTickHost(Engine& engine, Entity* hostEntity) {
	if (rootEntity != nullptr && (rootEntity->Flag().load(std::memory_order_acquire) & Entity::ENTITY_HAS_SPACE)) {
		UpdateTaskData(engine, hostEntity);
	}
}

void CameraComponent::CollectRenderableComponent(Engine& engine, TaskData& taskData, RenderableComponent* renderableComponent, TaskData::WarpData& warpData, const WorldInstanceData& instanceData) {
	IRender& render = engine.interfaces.render;
	IRender::Device* device = engine.snowyStream.GetRenderDevice();
	IDrawCallProvider::InputRenderData inputRenderData(instanceData.viewReference, nullptr, renderFlowComponent->GetMainResolution());
	std::vector<IDrawCallProvider::OutputRenderData> drawCalls;
	renderableComponent->CollectDrawCalls(drawCalls, inputRenderData);
	TaskData::WarpData::InstanceGroupMap& instanceGroups = warpData.instanceGroups;

	bool isCameraViewSpace = !!(renderableComponent->Flag().load(std::memory_order_relaxed) & RenderableComponent::RENDERABLECOMPONENT_CAMERAVIEW);

	for (size_t k = 0; k < drawCalls.size(); k++) {
		// PassBase& Pass = provider->GetPass(k);
		IDrawCallProvider::OutputRenderData& drawCall = drawCalls[k];
		uint32_t div;
		switch (drawCall.drawCallDescription.indexBufferResource.type) {
		case IRender::Resource::BufferDescription::Format::UNSIGNED_BYTE:
			div = 3;
			break;
		case IRender::Resource::BufferDescription::Format::UNSIGNED_SHORT:
			div = 6;
			break;
		default:
			div = 12;
			break;
		}

		warpData.triangleCount += drawCall.drawCallDescription.indexBufferResource.length / div * Math::Max(1u, drawCall.drawCallDescription.instanceCounts.x());

		const IRender::Resource::DrawCallDescription& drawCallTemplate = drawCall.drawCallDescription;
		AnimationComponent* animationComponent = instanceData.animationComponent();

		// Add Lighting stencil
		assert(!(drawCall.renderStateDescription.stencilValue & STENCIL_LIGHTING));
		drawCall.renderStateDescription.stencilValue |= STENCIL_LIGHTING;
		drawCall.renderStateDescription.stencilMask |= STENCIL_LIGHTING;

		// Generate key
		InstanceKey key;
		key.renderStateDescription = drawCall.renderStateDescription;
		assert((((size_t)renderableComponent << 1) & k) == 0);
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

			RenderPolicy* renderPolicy = renderableComponent->renderPolicy();
			group.renderPolicy = renderPolicy;

			std::vector<std::key_value<RenderPolicy*, TaskData::PolicyData> >::iterator ip = std::binary_find(warpData.renderPolicyMap.begin(), warpData.renderPolicyMap.end(), group.renderPolicy());
			if (ip == warpData.renderPolicyMap.end()) {
				ip = std::binary_insert(warpData.renderPolicyMap, group.renderPolicy());
			}

			TaskData::PolicyData& policyData = ip->second;
			IRender::Queue*& queue = policyData.portQueue;
			if (queue == nullptr) {
				queue = render.CreateQueue(device);
				policyData.instanceBuffer = render.CreateResource(render.GetQueueDevice(queue), IRender::Resource::RESOURCE_BUFFER);
			}

			// add renderstate if exists
			std::vector<std::key_value<IRender::Resource::RenderStateDescription, IRender::Resource*> >::iterator is = std::binary_find(warpData.renderStateMap.begin(), warpData.renderStateMap.end(), drawCall.renderStateDescription);
			if (is == warpData.renderStateMap.end()) {
				is = std::binary_insert(warpData.renderStateMap, drawCall.renderStateDescription);
			}

			IRender::Resource*& state = is->second;
			if (state == nullptr) {
				state = render.CreateResource(render.GetQueueDevice(queue), IRender::Resource::RESOURCE_RENDERSTATE);
				render.UploadResource(queue, state, &drawCall.renderStateDescription);
				policyData.runtimeResources.emplace_back(state);
			}

			group.renderStateResource = state;
			group.drawCallDescription = drawCallTemplate;

#ifdef _DEBUG
			group.description = renderableComponent->GetDescription();
#endif // _DEBUG

			PassBase::Updater& updater = drawCall.shaderResource->GetPassUpdater();
			std::vector<std::key_value<ShaderResource*, TaskData::WarpData::GlobalBufferItem> >::iterator ig = std::binary_find(warpData.worldGlobalBufferMap.begin(), warpData.worldGlobalBufferMap.end(), drawCall.shaderResource());
			if (ig == warpData.worldGlobalBufferMap.end()) {
				ig = std::binary_insert(warpData.worldGlobalBufferMap, drawCall.shaderResource());

				ig->second.renderQueue = queue;
				taskData.worldGlobalData.Export(ig->second.globalUpdater, updater);
				// assert(!ig->second.globalUpdater.parameters.empty());
				instanceData.Export(ig->second.instanceUpdater, updater);
				// assert(!ig->second.instanceUpdater.parameters.empty());

				std::vector<Bytes> s;
				std::vector<IRender::Resource::DrawCallDescription::BufferRange> bufferResources;
				std::vector<IRender::Resource*> textureResources;
				ig->second.globalUpdater.Snapshot(s, bufferResources, textureResources, taskData.worldGlobalData);
				ig->second.buffers.resize(updater.GetBufferCount());

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
						ig->second.buffers[i] = res;
						policyData.runtimeResources.emplace_back(res);
					}
				}
			}

			for (size_t n = 0; n < group.drawCallDescription.bufferResources.size(); n++) {
				IRender::Resource::DrawCallDescription::BufferRange& bufferRange = group.drawCallDescription.bufferResources[n];
				if (ig->second.buffers[n] != nullptr) {
					bufferRange.buffer = ig->second.buffers[n];
					bufferRange.offset = bufferRange.length = 0;
				}
			}

			group.instanceUpdater = &ig->second.instanceUpdater;

			// skinning
			if (animationComponent) {
				assert(animationComponent->GetWarpIndex() == renderableComponent->GetWarpIndex());
				const PassBase::Parameter& parameter = updater[IShader::BindInput::BONE_TRANSFORMS];
				if (parameter) {
					group.animationComponent = animationComponent; // hold reference
					group.drawCallDescription.bufferResources[parameter.slot].buffer = animationComponent->AcquireBoneMatrixBuffer(render, queue);
				}
			}
		}

		// process local instanced data
		if (drawCall.localInstancedData.empty() && drawCall.localTransforms.empty()) {
			group.instanceUpdater->Snapshot(group.instancedData, bufferResources, textureResources, instanceData);
			assert(!group.instanceUpdater->parameters.empty());
			group.instanceCount++;
		} else {
			assert(drawCall.drawCallDescription.instanceCounts.x() != 0);

			for (size_t i = 0; i < drawCall.localInstancedData.size(); i++) {
				std::pair<uint32_t, Bytes>& localInstancedData = drawCall.localInstancedData[i];
				if (group.instancedData.size() <= localInstancedData.first) {
					group.instancedData.resize(localInstancedData.first + 1);
				}

				group.instancedData[localInstancedData.first].Append(localInstancedData.second);
			}

			if (drawCall.localTransforms.empty()) {
				std::vector<Bytes> s;
				group.instanceUpdater->Snapshot(s, bufferResources, textureResources, instanceData);

				for (size_t j = 0; j < s.size(); j++) {
					for (size_t n = 0; n < drawCall.drawCallDescription.instanceCounts.x(); n++) {
						group.instancedData[j].Append(s[j]);
					}
				}
			} else {
				assert(drawCall.drawCallDescription.instanceCounts.x() == drawCall.localTransforms.size());
				for (size_t n = 0; n < drawCall.drawCallDescription.instanceCounts.x(); n++) {
					WorldInstanceData subInstanceData = instanceData;
					subInstanceData.worldMatrix = drawCall.localTransforms[n] * instanceData.worldMatrix;
					if (isCameraViewSpace) {
						subInstanceData.worldMatrix = subInstanceData.worldMatrix * taskData.worldGlobalData.viewMatrix;
					}
					group.instanceUpdater->Snapshot(group.instancedData, bufferResources, textureResources, subInstanceData);
				}
			}

			group.instanceCount += drawCall.drawCallDescription.instanceCounts.x();
		}
	}
}

void CameraComponent::CollectEnvCubeComponent(EnvCubeComponent* envCubeComponent, std::vector<std::pair<TShared<RenderPolicy>, EnvCubeElement> >& envCubeElements, const MatrixFloat4x4& worldMatrix) const {
	EnvCubeElement element;
	element.position = Float3(worldMatrix(3, 0), worldMatrix(3, 1), worldMatrix(3, 2));
	element.cubeMapTexture = envCubeComponent->cubeMapTexture;
	element.cubeStrength = envCubeComponent->strength;
	envCubeElements.emplace_back(std::make_pair(envCubeComponent->renderPolicy, std::move(element)));
}

void CameraComponent::CompleteCollect(Engine& engine, TaskData& taskData) {
	Kernel& kernel = engine.GetKernel();
	Flag().fetch_and(~CAMERACOMPONENT_UPDATE_COLLECTING, std::memory_order_release);

	if (kernel.GetCurrentWarpIndex() != GetWarpIndex()) {
		kernel.QueueRoutine(this, CreateTaskContextFree(Wrap(this, &CameraComponent::CompleteCollect), std::ref(engine), std::ref(taskData)));
	} else {
		Instancing(engine, taskData);
		Flag().fetch_or(CAMERACOMPONENT_UPDATE_COLLECTED, std::memory_order_acq_rel);
	}
}

void CameraComponent::CollectLightComponent(Engine& engine, LightComponent* lightComponent, std::vector<std::pair<TShared<RenderPolicy>, LightElement> >& lightElements, const MatrixFloat4x4& worldMatrix, const TaskData& taskData) const {
	LightElement element;
	if (lightComponent->Flag().load(std::memory_order_relaxed) & LightComponent::LIGHTCOMPONENT_DIRECTIONAL) {
		element.position = Float4(-worldMatrix(2, 0), -worldMatrix(2, 1), -worldMatrix(2, 2), 0);

		// refresh shadow
		std::vector<TShared<LightComponent::ShadowGrid> > shadowGrids = lightComponent->UpdateShadow(engine, taskData.worldGlobalData.inverseViewMatrix, worldMatrix, rootEntity);
		for (size_t i = 0; i < shadowGrids.size(); i++) {
			TShared<LightComponent::ShadowGrid>& grid = shadowGrids[i];
			RenderPortLightSource::LightElement::Shadow shadow;
			shadow.shadowTexture = grid->Flag().load(std::memory_order_relaxed) & TINY_MODIFIED ? nullptr : grid->texture;
			shadow.shadowMatrix = grid->shadowMatrix;
			element.shadows.emplace_back(std::move(shadow));
		}
	} else {
		float range = lightComponent->GetRange().x();
		element.position = Float4(worldMatrix(3, 0), worldMatrix(3, 1), worldMatrix(3, 2), Math::Max(0.05f, range * range));
	}

	const Float3& color = lightComponent->GetColor();
	element.colorAttenuation = Float4(color.x(), color.y(), color.z(), lightComponent->GetAttenuation());
	lightElements.emplace_back(std::make_pair(lightComponent->renderPolicy, std::move(element)));
}

uint32_t CameraComponent::GetCollectedEntityCount() const {
	return collectedEntityCount;
}

uint32_t CameraComponent::GetCollectedVisibleEntityCount() const {
	return collectedVisibleEntityCount;
}

uint32_t CameraComponent::GetCollectedTriangleCount() const {
	return collectedTriangleCount;
}

void CameraComponent::CollectComponents(Engine& engine, TaskData& taskData, const WorldInstanceData& instanceData, const CaptureData& captureData, Entity* entity) {
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
		// Fetch screen ratio
		float tanHalfFov = taskData.worldGlobalData.tanHalfFov;

		Float3 start = Math::Transform3D(instanceData.worldMatrix, localBoundingBox.first);
		Float3 end = Math::Transform3D(instanceData.worldMatrix, localBoundingBox.second);
		Float3 center = (start + end) * 0.5f;
		float length = Math::SquareLength(end - start);

		// Project
		subWorldInstancedData.viewReference = Math::Max(0.0f, 1.0f - length / (0.001f + tanHalfFov * Math::SquareLength(center - captureData.GetPosition())));
	}

	if (rootFlag & (Entity::ENTITY_HAS_RENDERABLE | Entity::ENTITY_HAS_RENDERCONTROL | Entity::ENTITY_HAS_SPACE)) {
		// optional animation
		subWorldInstancedData.animationComponent = entity->GetUniqueComponent(UniqueType<AnimationComponent>());

		std::vector<Component*> exploredComponents;
		ExplorerComponent* explorerComponent = entity->GetUniqueComponent(UniqueType<ExplorerComponent>());
		if (explorerComponent != nullptr) {
			// Use nearest refValue for selecting most detailed components
			explorerComponent->SelectComponents(engine, entity, subWorldInstancedData.viewReference, exploredComponents);
		}

		const std::vector<Component*>& components = explorerComponent != nullptr ? exploredComponents : entity->GetComponents();

		for (size_t i = 0; i < components.size(); i++) {
			Component* component = components[i];
			if (component == nullptr) continue;
			assert(component->Flag().load(std::memory_order_relaxed) & Tiny::TINY_ACTIVATED);
			if (!(component->Flag().load(std::memory_order_relaxed) & Tiny::TINY_ACTIVATED)) continue;

			Unique unique = component->GetUnique();

			// Since EntityMask would be much more faster than Reflection
			// We asserted that flaged components must be derived from specified implementations
			Tiny::FLAG entityMask = component->GetEntityFlagMask();

			if (entityMask & Entity::ENTITY_HAS_RENDERABLE) {
				bool isRenderControl = !!(entityMask & Entity::ENTITY_HAS_RENDERCONTROL);
				if (visible) {
					assert(component->QueryInterface(UniqueType<RenderableComponent>()) != nullptr);
					CollectRenderableComponent(engine, taskData, static_cast<RenderableComponent*>(component), warpData, subWorldInstancedData);
					if (!isRenderControl) {
						++warpData.visibleEntityCount;
					}
				}

				if (isRenderControl) {
					LightComponent* lightComponent;
					EnvCubeComponent* envCubeComponent;
					if ((lightComponent = component->QueryInterface(UniqueType<LightComponent>())) != nullptr) {
						CollectLightComponent(engine, lightComponent, warpData.lightElements, subWorldInstancedData.worldMatrix, taskData);
					} else if ((envCubeComponent = component->QueryInterface(UniqueType<EnvCubeComponent>())) != nullptr) {
						CollectEnvCubeComponent(envCubeComponent, warpData.envCubeElements, subWorldInstancedData.worldMatrix);
					}
				} else {
					++warpData.entityCount;
				}
			} else if (entityMask & Entity::ENTITY_HAS_SPACE) {
				assert(component->QueryInterface(UniqueType<SpaceComponent>()) != nullptr);
				WorldInstanceData subSpaceWorldInstancedData = subWorldInstancedData;
				subSpaceWorldInstancedData.animationComponent = nullptr; // animation info cannot be derived

				VisibilityComponent* visibilityComponent = entity->GetUniqueComponent(UniqueType<VisibilityComponent>());

				taskData.pendingCount.fetch_add(1, std::memory_order_release);

				CaptureData newCaptureData = captureData;
				SpaceComponent* spaceComponent = static_cast<SpaceComponent*>(component);
				bool captureFree = !!(spaceComponent->GetEntityFlagMask() & Entity::ENTITY_HAS_RENDERCONTROL);
				const MatrixFloat4x4& mat = captureData.viewTransform;

				if (transformComponent != nullptr) {
					UpdateCaptureData(newCaptureData, mat * Math::QuickInverse(localTransform));
				}

				const MatrixFloat4x4& newMat = newCaptureData.viewTransform;
				const Bytes& visData = visibilityComponent != nullptr ? visibilityComponent->QuerySample(engine, Float3(newMat(3, 0), newMat(3, 1), newMat(3, 2))) : Bytes::Null();
				newCaptureData.visData = visData;

				CollectComponentsFromSpace(engine, taskData, subSpaceWorldInstancedData, newCaptureData, spaceComponent);
			}
		}
	}
}

void CameraComponent::BindRootEntity(Engine& engine, Entity* entity) {
	if (rootEntity != nullptr) {
		// free last listener
		rootEntity->RemoveComponent(engine, this);
	}

	rootEntity = entity;

	if (entity != nullptr) {
		entity->AddComponent(engine, this); // weak component
	}
}

CameraComponentConfig::TaskData::TaskData(uint32_t warpCount) {
	warpData.resize(warpCount);
	pendingCount.store(0, std::memory_order_relaxed);
}

TObject<IReflect>& CameraComponent::TaskData::operator () (IReflect& reflect) {
	BaseClass::operator () (reflect);

	if (reflect.IsReflectProperty()) {
		ReflectProperty(worldGlobalData);
	}

	return *this;
}

void CameraComponent::TaskData::Cleanup(IRender& render) {
	for (size_t i = 0; i < warpData.size(); i++) {
		WarpData& data = warpData[i];
		for (std::vector<std::key_value<RenderPolicy*, PolicyData> >::iterator it = data.renderPolicyMap.begin(); it != data.renderPolicyMap.end(); ++it) {
			PolicyData& policyData = it->second;
			IRender::Queue* queue = policyData.portQueue;

			for (size_t k = 0; k < policyData.runtimeResources.size(); k++) {
				render.DeleteResource(queue, policyData.runtimeResources[k]);
			}

			policyData.runtimeResources.clear();
		}

		data.renderStateMap.clear();
		data.worldGlobalBufferMap.clear();
		data.dataUpdaters.clear();
		data.envCubeElements.clear();
		data.lightElements.clear();
		data.entityCount = 0;
		data.visibleEntityCount = 0;
		data.triangleCount = 0;

		// to avoid frequently memory alloc/dealloc
		// data.instanceGroups.clear();
		for (WarpData::InstanceGroupMap::iterator ip = data.instanceGroups.begin(); ip != data.instanceGroups.end();) {
			InstanceGroup& group = (*ip).second;
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

void CameraComponent::InstanceGroup::Cleanup() {
	for (size_t i = 0; i < instancedData.size(); i++) {
		instancedData[i].Clear();
	}

	drawCallDescription = IRender::Resource::DrawCallDescription();
	drawCallResource = nullptr;
	instanceCount = 0;
}

CameraComponent::TaskData::~TaskData() {
	assert(warpData.empty());
}

CameraComponent::TaskData::PolicyData::PolicyData() : portQueue(nullptr) {}

void CameraComponent::TaskData::Destroy(IRender& render) {
	Cleanup(render);

	for (size_t i = 0; i < warpData.size(); i++) {
		WarpData& data = warpData[i];
		for (std::vector<std::key_value<RenderPolicy*, PolicyData> >::iterator it = data.renderPolicyMap.begin(); it != data.renderPolicyMap.end(); ++it) {
			render.DeleteResource(it->second.portQueue, it->second.instanceBuffer);
			render.DeleteQueue(it->second.portQueue);
		}
	}

	warpData.clear();
}

TObject<IReflect>& CameraComponent::operator () (IReflect& reflect) {
	BaseClass::operator () (reflect);
	if (reflect.IsReflectProperty()) {
		ReflectProperty(prevTaskData)[Runtime];
		ReflectProperty(nextTaskData)[Runtime];
		ReflectProperty(renderFlowComponent)[Runtime];
		ReflectProperty(rootEntity)[Runtime];

		ReflectProperty(collectedEntityCount);
		ReflectProperty(collectedVisibleEntityCount);
		ReflectProperty(collectedTriangleCount);
		ReflectProperty(jitterIndex)[Runtime];

		ReflectProperty(nearPlane);
		ReflectProperty(farPlane);
		ReflectProperty(fov);
		ReflectProperty(aspect);
		ReflectProperty(viewDistance);
	}

	return *this;
}

TObject<IReflect>& CameraComponent::WorldInstanceData::operator () (IReflect& reflect) {
	BaseClass::operator () (reflect);

	if (reflect.IsReflectProperty()) {
		ReflectProperty(worldMatrix)[IShader::BindInput(IShader::BindInput::TRANSFORM_WORLD)];
	}

	return *this;
}

TObject<IReflect>& CameraComponentConfig::WorldGlobalData::operator () (IReflect& reflect) {
	BaseClass::operator () (reflect);

	if (reflect.IsReflectProperty()) {
		ReflectProperty(viewProjectionMatrix)[IShader::BindInput(IShader::BindInput::TRANSFORM_VIEWPROJECTION)];
		ReflectProperty(lastViewProjectionMatrix)[IShader::BindInput(IShader::BindInput::TRANSFORM_LAST_VIEWPROJECTION)];
		ReflectProperty(viewMatrix)[IShader::BindInput(IShader::BindInput::TRANSFORM_VIEW)];
		ReflectProperty(viewPosition);
		ReflectProperty(time);
		ReflectProperty(tanHalfFov);
	}

	return *this;
}
