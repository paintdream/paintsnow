#include "RenderResourceManager.h"

#include "../Resource/ShaderResource.h"
#include "../Resource/FontResource.h"
#include "../Resource/MaterialResource.h"
#include "../Resource/MeshResource.h"
#include "../Resource/Passes/AntiAliasingPass.h"
#include "../Resource/Passes/BloomPass.h"
#include "../Resource/Passes/ConstMapPass.h"
#include "../Resource/Passes/CustomMaterialPass.h"
#include "../Resource/Passes/DeferredLightingBufferEncodedPass.h"
#include "../Resource/Passes/DeferredLightingTextureEncodedPass.h"
#include "../Resource/Passes/DepthResolvePass.h"
#include "../Resource/Passes/DepthBoundingPass.h"
#include "../Resource/Passes/DepthBoundingSetupPass.h"
#include "../Resource/Passes/ForwardLightingPass.h"
#include "../Resource/Passes/LightBufferEncodePass.h"
#include "../Resource/Passes/LightTextureEncodePass.h"
#include "../Resource/Passes/MultiHashSetupPass.h"
#include "../Resource/Passes/MultiHashTracePass.h"
#include "../Resource/Passes/ParticlePass.h"
#include "../Resource/Passes/ScreenPass.h"
#include "../Resource/Passes/ShadowMaskPass.h"
#include "../Resource/Passes/SkyDirectIrradiancePass.h"
#include "../Resource/Passes/SkyIndirectIrradiancePass.h"
#include "../Resource/Passes/SkyMultipleScatteringPass.h"
#include "../Resource/Passes/SkyPass.h"
#include "../Resource/Passes/SkyScatteringDensityPass.h"
#include "../Resource/Passes/SkySingleScatteringPass.h"
#include "../Resource/Passes/SkyTransmittancePass.h"
#include "../Resource/Passes/StandardPass.h"
#include "../Resource/Passes/TerrainPass.h"
#include "../Resource/Passes/TextPass.h"
#include "../Resource/Passes/VolumePass.h"
#include "../Resource/Passes/WaterPass.h"
#include "../Resource/Passes/WidgetPass.h"
#include "../../../Core/Driver/Profiler/Optick/optick.h"

using namespace PaintsNow;

RenderResourceManager::RenderResourceManager(ThreadPool& tp, IUniformResourceManager& hostManager, IRender& dev, const TWrapper<void, const String&>& errorHandler, void* c) : DeviceResourceManager<IRender>(tp, hostManager, dev, errorHandler, c), resourceQueue(nullptr), renderResourceStepPerFrame(0) {
	renderDevice = dev.CreateDevice("");
	resourceQueue = dev.CreateQueue(renderDevice, IRender::QUEUE_MULTITHREAD);
	assert(context == nullptr); // must not initialize context
	context = resourceQueue;
	currentNotifiedResourceCount.store(0, std::memory_order_release);
	RegisterBuiltinPasses();
	RegisterBuiltinResources();
}

void RenderResourceManager::CreateBuiltinSolidTexture(const String& path, const UChar4& color) {
	IRender& render = device;
	// Error Texture for missing textures ...
	TShared<TextureResource> textureResource = TShared<TextureResource>::From(new TextureResource(*this, path));
	textureResource->Flag().fetch_or(ResourceBase::RESOURCE_ETERNAL | ResourceBase::RESOURCE_VIRTUAL);
	DoLock();
	Insert(textureResource());
	UnLock();

	textureResource->description.state.format = IRender::Resource::TextureDescription::UNSIGNED_BYTE;
	textureResource->description.state.layout = IRender::Resource::TextureDescription::RGBA;
	// 2x2 pixel
	const int width = 2, height = 2;
	textureResource->description.dimension.x() = width;
	textureResource->description.dimension.y() = height;
	textureResource->description.data.Resize(width * height * sizeof(UChar4));

	UChar4* buffer = reinterpret_cast<UChar4*>(textureResource->description.data.GetData());
	std::fill(buffer, buffer + width * height, color);

	textureResource->Flag().fetch_or(Tiny::TINY_MODIFIED, std::memory_order_release);
	textureResource->GetResourceManager().InvokeUpload(textureResource());
}

void RenderResourceManager::CreateBuiltinMesh(const String& path, const Float3* vertices, size_t vertexCount, const UInt3* indices, size_t indexCount) {
	TShared<MeshResource> meshResource = TShared<MeshResource>::From(new MeshResource(*this, path));
	meshResource->Flag().fetch_or(ResourceBase::RESOURCE_ETERNAL | ResourceBase::RESOURCE_VIRTUAL);
	DoLock();
	Insert(meshResource());
	UnLock();

	IRender& render = device;

	meshResource->meshCollection.vertices.assign(vertices, vertices + vertexCount);
	meshResource->meshCollection.indices.assign(indices, indices + indexCount);

	// add mesh group
	IAsset::MeshGroup group;
	group.primitiveOffset = 0;
	group.primitiveCount = 2;
	meshResource->meshCollection.groups.emplace_back(group);
	meshResource->Flag().fetch_or(Tiny::TINY_MODIFIED, std::memory_order_release);
	meshResource->GetResourceManager().InvokeUpload(meshResource());
}

// https://github.com/caosdoar/spheres/blob/master/src/spheres.cpp
static void SpherifiedCube(uint32_t divisions, std::vector<UInt3>& indices, std::vector<Float3>& vertices) {
	static const Float3 origins[6] = {
		Float3(-1.0, -1.0, -1.0),
		Float3(1.0, -1.0, -1.0),
		Float3(1.0, -1.0, 1.0),
		Float3(-1.0, -1.0, 1.0),
		Float3(-1.0, 1.0, -1.0),
		Float3(-1.0, -1.0, 1.0)
	};

	static const Float3 rights[6] = {
		Float3(2.0, 0.0, 0.0),
		Float3(0.0, 0.0, 2.0),
		Float3(-2.0, 0.0, 0.0),
		Float3(0.0, 0.0, -2.0),
		Float3(2.0, 0.0, 0.0),
		Float3(2.0, 0.0, 0.0)
	};

	static const Float3 ups[6] =
	{
		Float3(0.0, 2.0, 0.0),
		Float3(0.0, 2.0, 0.0),
		Float3(0.0, 2.0, 0.0),
		Float3(0.0, 2.0, 0.0),
		Float3(0.0, 0.0, 2.0),
		Float3(0.0, 0.0, -2.0)
	};

	const float step = 1.0f / divisions;
	vertices.reserve(6 * (divisions + 1) * (divisions + 1));

	uint32_t face;
	for (face = 0; face < 6; face++) {
		const Float3 origin = origins[face];
		const Float3 right = rights[face];
		const Float3 up = ups[face];
		for (uint32_t j = 0; j < divisions + 1; j++) {
			for (uint32_t i = 0; i < divisions + 1; i++) {
				const Float3 p = origin + (right * (float)i + up * (float)j) * step;
				const Float3 p2 = p * p;
				const Float3 n
				(
					p.x() * (float)sqrt(1.0f - 0.5f * (p2.y() + p2.z()) + p2.y() * p2.z() / 3.0f),
					p.y() * (float)sqrt(1.0f - 0.5f * (p2.z() + p2.x()) + p2.z() * p2.x() / 3.0f),
					p.z() * (float)sqrt(1.0f - 0.5f * (p2.x() + p2.y()) + p2.x() * p2.y() / 3.0f)
				);

				vertices.emplace_back(n);
			}
		}
	}

	indices.reserve(6 * divisions * divisions);
	const uint32_t k = divisions + 1;
	for (face = 0; face < 6; ++face) {
		for (uint32_t j = 0; j < divisions; ++j) {
			const bool bottom = j < (divisions / 2);
			for (uint32_t i = 0; i < divisions; ++i) {
				const bool left = i < (divisions / 2);
				const uint32_t a = (face * k + j) * k + i;
				const uint32_t b = (face * k + j) * k + i + 1;
				const uint32_t c = (face * k + j + 1) * k + i;
				const uint32_t d = (face * k + j + 1) * k + i + 1;
				if (bottom != left) {
					indices.emplace_back(UInt3(a, c, b));
					indices.emplace_back(UInt3(c, d, b));
				} else {
					indices.emplace_back(UInt3(a, c, d));
					indices.emplace_back(UInt3(a, d, b));
				}
			}
		}
	}
}


void RenderResourceManager::RegisterBuiltinResources() {
	// MeshResource for widget rendering and deferred rendering ...
	static const Float3 quadVertices[] = {
		Float3(-1.0f, -1.0f, 0.0f),
		Float3(1.0f, -1.0f, 0.0f),
		Float3(1.0f, 1.0f, 0.0f),
		Float3(-1.0f, 1.0f, 0.0f),
	};

	static const UInt3 quadIndices[] = { UInt3(0, 1, 2), UInt3(2, 3, 0) };

	CreateBuiltinMesh("[Runtime]/MeshResource/StandardQuad", quadVertices, sizeof(quadVertices) / sizeof(quadVertices[0]), quadIndices, sizeof(quadIndices) / sizeof(quadIndices[0]));

	static const Float3 cubeVertices[] = {
		Float3(-1.0f, -1.0f, -1.0f),
		Float3(1.0f, -1.0f, -1.0f),
		Float3(1.0f, 1.0f, -1.0f),
		Float3(-1.0f, 1.0f, -1.0f),
		Float3(-1.0f, -1.0f, 1.0f),
		Float3(1.0f, -1.0f, 1.0f),
		Float3(1.0f, 1.0f, 1.0f),
		Float3(-1.0f, 1.0f, 1.0f),
	};

	static const UInt3 cubeIndices[] = {
		UInt3(0, 2, 1), UInt3(2, 0, 3), // bottom
		UInt3(0, 4, 7), UInt3(0, 7, 3), // left
		UInt3(0, 5, 4), UInt3(0, 1, 5), // back
		UInt3(4, 5, 6), UInt3(6, 7, 4), // top
		UInt3(1, 6, 5), UInt3(1, 2, 6), // right
		UInt3(3, 6, 2), UInt3(3, 7, 6), // front
	};

	CreateBuiltinMesh("[Runtime]/MeshResource/StandardCube", cubeVertices, sizeof(cubeVertices) / sizeof(cubeVertices[0]), cubeIndices, sizeof(cubeIndices) / sizeof(cubeIndices[0]));

	std::vector<UInt3> sphereIndices;
	std::vector<Float3> sphereVertices;
	SpherifiedCube(5, sphereIndices, sphereVertices);
	CreateBuiltinMesh("[Runtime]/MeshResource/StandardSphere", &sphereVertices[0], sphereVertices.size(), &sphereIndices[0], sphereIndices.size());

	CreateBuiltinSolidTexture("[Runtime]/TextureResource/Black", UChar4(0, 0, 0, 0));
	CreateBuiltinSolidTexture("[Runtime]/TextureResource/White", UChar4(255, 255, 255, 255));
	CreateBuiltinSolidTexture("[Runtime]/TextureResource/MissingBaseColor", UChar4(255, 0, 255, 255));
	CreateBuiltinSolidTexture("[Runtime]/TextureResource/MissingNormal", UChar4(127, 127, 255, 255));
	CreateBuiltinSolidTexture("[Runtime]/TextureResource/MissingMaterial", UChar4(255, 255, 0, 0));
}

IRender::Device* RenderResourceManager::GetRenderDevice() const {
	return renderDevice;
}

RenderResourceManager::~RenderResourceManager() {
	device.DeleteQueue(resourceQueue);
	resourceQueue = nullptr;
	device.DeleteDevice(renderDevice);
	renderDevice = nullptr;
}

template <class T>
void RegisterPass(ResourceManager& resourceManager, UniqueType<T> type, const String& matName = "", const IRender::Resource::RenderStateDescription& renderState = IRender::Resource::RenderStateDescription()) {
	ShaderResourceImpl<T>* shaderResource = new ShaderResourceImpl<T>(resourceManager, "", ResourceBase::RESOURCE_ETERNAL | ResourceBase::RESOURCE_VIRTUAL);
	PassBase& pass = shaderResource->GetPass();
	shaderResource->GetPassUpdater().Initialize(pass);

	Unique unique = pass.GetUnique();
	assert(unique != UniqueType<PassBase>().Get());
	String name = pass.GetUnique()->GetName();
	auto pos = name.find_last_of(':');
	if (pos != String::npos) {
		name = name.substr(pos + 1);
	}

	resourceManager.DoLock();
	shaderResource->SetLocation(ShaderResource::GetShaderPathPrefix() + name);
	resourceManager.Insert(shaderResource);
	resourceManager.UnLock();

	if (!matName.empty()) {
		TShared<MaterialResource> materialResource = TShared<MaterialResource>::From(new MaterialResource(resourceManager, String("[Runtime]/MaterialResource/") + matName));
		materialResource->originalShaderResource = shaderResource;
		materialResource->materialParams.state = renderState;
		materialResource->Flag().fetch_or(ResourceBase::RESOURCE_ETERNAL | ResourceBase::RESOURCE_VIRTUAL, std::memory_order_release);

		resourceManager.DoLock();
		resourceManager.Insert(materialResource());
		resourceManager.UnLock();
	}

	shaderResource->ReleaseObject();
}

void RenderResourceManager::RegisterBuiltinPasses() {
	RegisterPass(*this, UniqueType<AntiAliasingPass>());
	RegisterPass(*this, UniqueType<BloomPass>());
	RegisterPass(*this, UniqueType<ConstMapPass>());
	RegisterPass(*this, UniqueType<CustomMaterialPass>());
	RegisterPass(*this, UniqueType<DeferredLightingBufferEncodedPass>());
	RegisterPass(*this, UniqueType<DeferredLightingTextureEncodedPass>());
	RegisterPass(*this, UniqueType<DepthResolvePass>());
	RegisterPass(*this, UniqueType<DepthBoundingPass>());
	RegisterPass(*this, UniqueType<DepthBoundingSetupPass>());
	RegisterPass(*this, UniqueType<ForwardLightingPass>());
	RegisterPass(*this, UniqueType<LightBufferEncodePass>());
	RegisterPass(*this, UniqueType<LightTextureEncodePass>());
	RegisterPass(*this, UniqueType<MultiHashSetupPass>());
	RegisterPass(*this, UniqueType<MultiHashTracePass>());
	RegisterPass(*this, UniqueType<ScreenPass>());
	RegisterPass(*this, UniqueType<ShadowMaskPass>());
	RegisterPass(*this, UniqueType<SkyDirectIrradiancePass>());
	RegisterPass(*this, UniqueType<SkyIndirectIrradiancePass>());
	RegisterPass(*this, UniqueType<SkyMultipleScatteringPass>());
	RegisterPass(*this, UniqueType<SkyPass>());
	RegisterPass(*this, UniqueType<SkyScatteringDensityPass>());
	RegisterPass(*this, UniqueType<SkySingleScatteringPass>());
	RegisterPass(*this, UniqueType<SkyTransmittancePass>());
	RegisterPass(*this, UniqueType<StandardPass>());

	IRender::Resource::RenderStateDescription state;
	state.depthTest = 0;
	RegisterPass(*this, UniqueType<TextPass>(), "Text", state);
	RegisterPass(*this, UniqueType<WidgetPass>(), "Widget", state);

	/*
	RegisterPass(*this, UniqueType<ParticlePass>());
	RegisterPass(*this, UniqueType<TerrainPass>());
	RegisterPass(*this, UniqueType<VolumePass>());
	RegisterPass(*this, UniqueType<WaterPass>());*/
}

IRender::Queue* RenderResourceManager::GetResourceQueue() {
	return resourceQueue;
}

uint32_t RenderResourceManager::GetRenderResourceFrameStep() const {
	return renderResourceStepPerFrame;
}

size_t RenderResourceManager::GetProfile(const String& feature) {
	return device.GetProfile(renderDevice, feature);
}

void RenderResourceManager::SetRenderResourceFrameStep(uint32_t limitStep) {
	renderResourceStepPerFrame = limitStep;
}

size_t RenderResourceManager::GetCurrentRuntimeVersion() const {
	return currentRuntimeVersion.load(std::memory_order_acquire);
}

size_t RenderResourceManager::GetNextRuntimeVersion() const {
	return nextRuntimeVersion.load(std::memory_order_acquire);
}

size_t RenderResourceManager::NotifyCompletion(const TShared<ResourceBase>& resource) {
	size_t limit = renderResourceStepPerFrame;
	uint32_t runtimeVersion = nextRuntimeVersion.load(std::memory_order_acquire);

	if (limit == 0) {
		// available at once!
		resource->Complete(runtimeVersion);
		return runtimeVersion;
	} else {
		DoLock();
		pendingCompletionResources.Push(resource);
		UnLock();

		uint32_t current = currentNotifiedResourceCount.fetch_add(1, std::memory_order_acquire) + 1;
		IRender& render = device;
		while (current >= limit && currentNotifiedResourceCount.compare_exchange_strong(current, current - limit, std::memory_order_release)) {
			nextRuntimeVersion.fetch_add(1, std::memory_order_acquire);
			DoLock();
			pendingCompletionResources.Push(TShared<ResourceBase>(nullptr));
			render.FlushQueue(resourceQueue); // yield execution to next frame(s)
			UnLock();
		}

		return runtimeVersion + 1;
	}
}

bool RenderResourceManager::GetCompleted() const {
	assert(currentRuntimeVersion.load(std::memory_order_acquire) <= nextRuntimeVersion.load(std::memory_order_acquire));
	return currentRuntimeVersion.load(std::memory_order_acquire) == nextRuntimeVersion.load(std::memory_order_acquire);
}

void RenderResourceManager::TickDevice(IDevice& tickingDevice) {
	if (&device == &tickingDevice) {
		OPTICK_EVENT();

		if (resourceQueue != nullptr) {
			device.SubmitQueues(&resourceQueue, 1, IRender::SUBMIT_EXECUTE);
			assert(currentRuntimeVersion.load(std::memory_order_acquire) <= nextRuntimeVersion.load(std::memory_order_acquire));

			if (currentRuntimeVersion.load(std::memory_order_acquire) != nextRuntimeVersion.load(std::memory_order_acquire)) {
				size_t version = currentRuntimeVersion.fetch_add(1, std::memory_order_acquire) + 1;

				while (!pendingCompletionResources.Empty()) {
					const TShared<ResourceBase>& resource = pendingCompletionResources.Top();
					if (!resource) {
						pendingCompletionResources.Pop();
						break;
					}

					resource->Complete(version);
					pendingCompletionResources.Pop();
				}
			}
		}
	}
}

TObject<IReflect>& RenderResourceManager::operator () (IReflect& reflect) {
	typedef BaseClass DeviceResourceManagerIRender;
	ReflectClass(RenderResourceManager)[ReflectInterface(DeviceResourceManagerIRender)];

	return *this;
}

