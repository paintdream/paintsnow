#include "RayTraceComponent.h"
#include "../Transform/TransformComponent.h"
#include "../Model/ModelComponent.h"
#include "../Space/SpaceComponent.h"
#include "../Shape/ShapeComponent.h"
#include "../EnvCube/EnvCubeComponent.h"
#include "../../../SnowyStream/Manager/RenderResourceManager.h"
using namespace PaintsNow;

const float EPSILON = 1e-6f;

RayTraceComponent::Context::Context(Engine& e) : engine(e), possibilityForGeometryLight(0.5f), possibilityCubeMapLight(0.5f) {
	completedPixelCount.store(0, std::memory_order_relaxed);
}

RayTraceComponent::Context::~Context() {
	for (size_t k = 0; k < mappedResources.size(); k++) {
		mappedResources[k]->UnMap();
	}
}

RayTraceComponent::RayTraceComponent() : captureSize(1920, 1080), superSample(4), tileSize(8), rayCount(256), maxBounceCount(4), cubeMapQuantization(4096), cubeMapQuantizationMultiplier(8), completedPixelCountSync(0), stepMinimal(0.05f), stepMaximal(1000.0f), maxEnvironmentRadiance(64.0f, 64.0f, 64.0f, 0.0f) {}

RayTraceComponent::~RayTraceComponent() {}

void RayTraceComponent::Configure(uint16_t s, uint16_t t, uint32_t r, uint32_t b) {
	superSample = s;
	tileSize = t;
	rayCount = r;
	maxBounceCount = b;
}

void RayTraceComponent::SetCaptureSize(const UShort2& size) {
	captureSize = size;
}

const UShort2& RayTraceComponent::GetCaptureSize() const {
	return captureSize;
}

size_t RayTraceComponent::GetCompletedPixelCount() const {
	return completedPixelCountSync;
}

size_t RayTraceComponent::GetTotalPixelCount() const {
	return captureSize.x() * captureSize.y();
}

static uint32_t ReverseBits(uint32_t bits) {
	bits = (bits << 16) | (bits >> 16);
	bits = ((bits & 0x00ff00ff) << 8) | ((bits & 0xff00ff00) >> 8);
	bits = ((bits & 0x0f0f0f0f) << 4) | ((bits & 0xf0f0f0f0) >> 4);
	bits = ((bits & 0x33333333) << 2) | ((bits & 0xcccccccc) >> 2);
	bits = ((bits & 0x55555555) << 1) | ((bits & 0xaaaaaaaa) >> 1);

	return bits;
}

static const float PI = 3.1415926f;
static Float2 Hammersley(uint32_t i, uint32_t total) {
	return Float2((float)i / total * 2 * PI, ReverseBits(i) * 2.3283064365386963e-10f);
}

void RayTraceComponent::Capture(Engine& engine, const TShared<CameraComponent>& cameraComponent, float averageLuminance) {
	assert(cameraComponent() != nullptr);
	completedPixelCountSync = 0;

	TShared<Context> context = TShared<Context>::From(new Context(engine));
	context->invAverageLuminance = 1.0f / averageLuminance;
	context->threadLocalCache.resize(engine.GetKernel().GetThreadPool().GetThreadCount());
	// Generate random sequence
	context->randomSequence.resize(rayCount);
	for (uint32_t k = 0; k < rayCount; k++) {
		context->randomSequence[k] = Hammersley(k, rayCount);
	}

	context->referenceCameraComponent = cameraComponent;
	CameraComponentConfig::WorldGlobalData& worldGlobalData = cameraComponent->GetTaskData()->worldGlobalData;
	context->view = worldGlobalData.viewPosition;
	const MatrixFloat4x4& viewMatrix = worldGlobalData.viewMatrix;
	context->right = Float3(viewMatrix(0, 0), viewMatrix(1, 0), viewMatrix(2, 0)) * cameraComponent->aspect;
	context->up = Float3(viewMatrix(0, 1), viewMatrix(1, 1), viewMatrix(2, 1));
	context->forward = -Float3(viewMatrix(0, 2), viewMatrix(1, 2), viewMatrix(2, 2));
	context->capturedTexture = engine.snowyStream.CreateReflectedResource(UniqueType<TextureResource>(), "", false, ResourceBase::RESOURCE_VIRTUAL);
	IRender::Resource::TextureDescription& description = context->capturedTexture->description;
	description.dimension = UShort3(captureSize.x(), captureSize.y(), 1);
	description.state.format = IRender::Resource::TextureDescription::UNSIGNED_BYTE;
	description.state.layout = IRender::Resource::TextureDescription::RGBA;
	description.data.Resize(captureSize.x() * captureSize.y() * sizeof(UChar4));

	// map manually
	context->capturedTexture->Flag().fetch_or(ResourceBase::RESOURCE_MAPPED, std::memory_order_relaxed);
	context->capturedTexture->GetMapCounter().fetch_add(1, std::memory_order_release);

	Entity* hostEntity = context->referenceCameraComponent->GetBridgeComponent()->GetHostEntity();
	const std::vector<Component*>& components = hostEntity->GetComponents();
	std::vector<SpaceComponent*> spaceComponents;
	for (size_t i = 0; i < components.size(); i++) {
		Component* component = components[i];
		if (component == nullptr) continue;

		if (component->GetEntityFlagMask() & Entity::ENTITY_HAS_SPACE) {
			context->rootSpaceComponents.emplace_back(static_cast<SpaceComponent*>(component));
		}
	}

	context->clock = ITimer::GetSystemClock();
	engine.GetKernel().GetThreadPool().Dispatch(CreateTaskContextFree(Wrap(this, &RayTraceComponent::RoutineRayTrace), context), 1);
}

static UChar4 FromFloat4(const Float4& v) {
	return UChar4(
		(uint8_t)(Math::Clamp(v.x(), 0.0f, 1.0f) * 255),
		(uint8_t)(Math::Clamp(v.y(), 0.0f, 1.0f) * 255),
		(uint8_t)(Math::Clamp(v.z(), 0.0f, 1.0f) * 255),
		(uint8_t)(Math::Clamp(v.w(), 0.0f, 1.0f) * 255)
	);
}

static Float4 ToFloat4(const UChar4& v) {
	return Float4(
		v.x() / 255.0f,
		v.y() / 255.0f,
		v.z() / 255.0f,
		v.w() / 255.0f
	);
}

static Float4 ToFloat4Signed(const UChar4& v) {
	return Float4(
		v.x() * 2.0f / 255.0f - 1.0f,
		v.y() * 2.0f / 255.0f - 1.0f,
		v.z() * 2.0f / 255.0f - 1.0f,
		v.w() * 2.0f / 255.0f - 1.0f
	);
}

static UChar4 ToneMapping(const Float4& color, float invAverageLuminance) {
	static const float valuesACESInputMat[16] = {
		0.59719f, 0.07600f, 0.02840f, 0,
		0.35458f, 0.90834f, 0.13383f, 0,
		0.04823f, 0.01566f, 0.83777f, 0,
		0, 0, 0, 0
	};

	static const MatrixFloat4x4 ACESInputMat(valuesACESInputMat);

	static const float valuesACESOutputMat[16] = {
		1.60475f, -0.10208f, -0.00327f, 0,
		-0.53108f, 1.10813f, -0.07276f, 0,
		-0.07367f, -0.00605f, 1.07602f, 0,
		0, 0, 0, 0
	};

	static const MatrixFloat4x4 ACESOutputMat(valuesACESOutputMat);

	const float A = 2.51f;
	const float B = 0.03f;
	const float C = 2.43f;
	const float D = 0.59f;
	const float E = 0.14f;

	Float4 target = Math::AllMax(Float4(0, 0, 0, 0), Float4(color * invAverageLuminance * ACESInputMat));
	target = target * (target * Float4(A, A, A, A) + Float4(B, B, B, B)) / (target * (target * Float4(C, C, C, C) + Float4(D, D, D, D)) + Float4(E, E, E, E));
	target = Math::AllClamp(Float4(target * ACESOutputMat), Float4(0, 0, 0, 0), Float4(1, 1, 1, 1));

	UChar4 output;
	for (size_t i = 0; i < 3; i++) {
		output[i] = (uint8_t)(255 * Math::Clamp(powf(target[i], 1.0f / 2.2f), 0.0f, 1.0f));
	}

	output[3] = 255;
	return output;
}

void RayTraceComponent::RoutineRenderTile(const TShared<Context>& context, size_t i, size_t j) {
	uint32_t threadIndex = context->engine.GetKernel().GetThreadPool().GetCurrentThreadIndex();
	BytesCache& bytesCache = context->threadLocalCache[threadIndex];

	uint32_t width = Math::Min((uint32_t)tileSize, (uint32_t)(captureSize.x() - i * tileSize));
	uint32_t height = Math::Min((uint32_t)tileSize, (uint32_t)(captureSize.y() - j * tileSize));
	const Float3& view = context->view;

	float sampleDiv = 1.0f / (superSample * superSample);
	float invAverageLuminance = context->invAverageLuminance;
	for (uint32_t m = 0; m < height; m++) {
		for (uint32_t n = 0; n < width; n++) {
			uint32_t px = verify_cast<uint32_t>(i) * tileSize + n;
			uint32_t py = verify_cast<uint32_t>(j) * tileSize + m;

			Float4 finalColor(0, 0, 0, 0);
			for (uint32_t t = 0; t < superSample; t++) {
				for (uint32_t r = 0; r < superSample; r++) {
					float x = ((r + 0.5f) / superSample + px) / captureSize.x() * 2.0f - 1.0f;
					float y = ((t + 0.5f) / superSample + py) / captureSize.y() * 2.0f - 1.0f;

					Float3 dir = context->forward + context->right * x + context->up * y;
					bytesCache.Reset();
					finalColor += PathTrace(context, Float3Pair(view, dir), bytesCache, 0);
				}
			}

			finalColor *= sampleDiv;
			finalColor.w() = 1;

			UChar4* ptr = reinterpret_cast<UChar4*>(context->capturedTexture->description.data.GetData());
			ptr[py * captureSize.x() + px] = ToneMapping(finalColor, invAverageLuminance); // assuming average color

			// finish one
			completedPixelCountSync = context->completedPixelCount.fetch_add(1, std::memory_order_release);
		}
	}

	if (context->completedPixelCount.load(std::memory_order_acquire) == captureSize.x() * captureSize.y()) {
		// Go finish!
		context->engine.GetKernel().QueueRoutine(this, CreateTaskContextFree(Wrap(this, &RayTraceComponent::RoutineComplete), context));
	}
}

void RayTraceComponent::RoutineComplete(const TShared<Context>& context) {
	int64_t diffTime = ITimer::GetSystemClock() - context->clock;
	printf("Trace completed in %6.3fs\n", (double)diffTime / 1000.0);

	if (!outputPath.empty() && context->capturedTexture() != nullptr) {
		IImage& image = context->engine.interfaces.image;
		IRender::Resource::TextureDescription& description = context->capturedTexture->description;
		IRender::Resource::TextureDescription::Layout layout = (IRender::Resource::TextureDescription::Layout)description.state.layout;
		IRender::Resource::TextureDescription::Format format = (IRender::Resource::TextureDescription::Format)description.state.format;

		uint64_t length;
		IStreamBase* stream = context->engine.interfaces.archive.Open(outputPath, true, length);
		IImage::Image* png = image.Create(description.dimension.x(), description.dimension.y(), layout, format);
		void* buffer = image.GetBuffer(png);
		const UChar4* src = reinterpret_cast<const UChar4*>(description.data.GetData());
		UChar4* dst = reinterpret_cast<UChar4*>(buffer);
		for (size_t k = 0; k < (size_t)description.dimension.x() * description.dimension.y(); k++) {
			UChar4 c = src[k];
			std::swap(c.x(), c.z());
			dst[k] = c;
		}
		// memcpy(buffer, description.data.GetData(), verify_cast<size_t>(description.data.GetSize()));
		image.Save(png, *stream, "png");
		image.Delete(png);
		// write png
		stream->Destroy();
	}

	capturedTexture = context->capturedTexture;
}

void RayTraceComponent::SetOutputPath(const String& p) {
	outputPath = p;
}

// Force collect, regardless warp situration
void RayTraceComponent::RoutineCollectTextures(const TShared<Context>& context, Entity* rootEntity, const MatrixFloat4x4& worldMatrix) {
	for (Entity* entity = rootEntity; entity != nullptr; entity = entity->Right()) {
		IMemory::PrefetchRead(entity->Left());
		IMemory::PrefetchRead(entity->Right());

		const std::vector<Component*>& components = entity->GetComponents();
		for (size_t i = 0; i < components.size(); i++) {
			Component* component = components[i];
			if (component == nullptr) continue;

			uint32_t mask = component->GetEntityFlagMask();
			TransformComponent* transformComponent = entity->GetUniqueComponent(UniqueType<TransformComponent>());
			MatrixFloat4x4 localTransform = MatrixFloat4x4::Identity();
			if (transformComponent != nullptr) {
				localTransform = transformComponent->GetTransform();
			}

			MatrixFloat4x4 currentTransform = localTransform * worldMatrix;

			// TODO: explorer component!
			if (mask & Entity::ENTITY_HAS_SPACE) {
				SpaceComponent* spaceComponent = static_cast<SpaceComponent*>(component);
				if (spaceComponent->GetRootEntity() != nullptr) {
					RoutineCollectTextures(context, spaceComponent->GetRootEntity(), currentTransform);
				}
			} else if (mask & Entity::ENTITY_HAS_RENDERABLE) {
				bool isRenderControl = !!(mask & Entity::ENTITY_HAS_RENDERCONTROL);

				if (isRenderControl) {
					LightComponent* lightComponent = component->QueryInterface(UniqueType<LightComponent>());
					if (lightComponent != nullptr) {
						LightElement element;
						Float3 color = lightComponent->GetColor();
						float atten = lightComponent->GetAttenuation();
						element.colorAttenuation = Float4(color.x(), color.y(), color.z(), atten);

						if (lightComponent->Flag().load(std::memory_order_relaxed) & LightComponent::LIGHTCOMPONENT_DIRECTIONAL) {
							element.position = Float4(-currentTransform(2, 0), -currentTransform(2, 1), -currentTransform(2, 2), 0);
						} else {
							float range = lightComponent->GetRange().x();
							element.position = Float4(currentTransform(3, 0), currentTransform(3, 1), currentTransform(3, 2), Math::Max(0.05f, range * range));
						}

						// TODO: directional light only by now
						if (lightComponent->Flag().load(std::memory_order_relaxed) & LightComponent::LIGHTCOMPONENT_DIRECTIONAL) {
							context->lightElements.emplace_back(std::move(element));
						}
					}

					EnvCubeComponent* envCubeComponent = component->QueryInterface(UniqueType<EnvCubeComponent>());
					if (envCubeComponent != nullptr) {
						context->cubeMapTexture = envCubeComponent->cubeMapTexture;
						context->cubeMapTexture->Map();
						context->mappedResources.emplace_back(context->cubeMapTexture());
					}
				} else {
					ModelComponent* modelComponent = component->QueryInterface(UniqueType<ModelComponent>());
					if (modelComponent != nullptr) {
						const std::vector<std::pair<uint32_t, TShared<MaterialResource> > >& materials = modelComponent->GetMaterials();
						for (size_t j = 0; j < materials.size(); j++) {
							const TShared<MaterialResource>& material = materials[j].second;
							// assume pbr material
							TShared<TextureResource> baseColorTexture;
							TShared<TextureResource> normalTexture;
							TShared<TextureResource> mixtureTexture;

							for (size_t k = 0; k < material->materialParams.variables.size(); k++) {
								const IAsset::Material::Variable& var = material->materialParams.variables[k];
								if (var.key == StaticBytes(baseColorTexture)) {
									baseColorTexture = material->textureResources[var.Parse(UniqueType<IAsset::TextureIndex>()).value];
								} else if (var.key == StaticBytes(normalTexture)) {
									normalTexture = material->textureResources[var.Parse(UniqueType<IAsset::TextureIndex>()).value];
								} else if (var.key == StaticBytes(mixtureTexture)) {
									mixtureTexture = material->textureResources[var.Parse(UniqueType<IAsset::TextureIndex>()).value];
								}
							}

							if (baseColorTexture && normalTexture && mixtureTexture) {
								context->mapEntityToResourceIndex[reinterpret_cast<size_t>(entity)] = (uint32_t)verify_cast<uint32_t>(context->mappedResources.size());
								SnowyStream snowyStream = context->engine.snowyStream;
								baseColorTexture = snowyStream.CreateReflectedResource(UniqueType<TextureResource>(), baseColorTexture->GetLocation() + "$", true, ResourceBase::RESOURCE_MANUAL_UPLOAD | ResourceBase::RESOURCE_MAPPED);
								normalTexture = snowyStream.CreateReflectedResource(UniqueType<TextureResource>(), normalTexture->GetLocation() + "$", true, ResourceBase::RESOURCE_MANUAL_UPLOAD | ResourceBase::RESOURCE_MAPPED);
								mixtureTexture = snowyStream.CreateReflectedResource(UniqueType<TextureResource>(), mixtureTexture->GetLocation() + "$", true, ResourceBase::RESOURCE_MANUAL_UPLOAD | ResourceBase::RESOURCE_MAPPED);
								context->mappedResources.emplace_back(baseColorTexture());
								context->mappedResources.emplace_back(normalTexture());
								context->mappedResources.emplace_back(mixtureTexture());
							}
						}
					}
				}
			}
		}

		if (entity->Left() != nullptr) {
			RoutineCollectTextures(context, entity->Left(), worldMatrix);
		}
	}
}

TShared<TextureResource> RayTraceComponent::GetCapturedTexture() const {
	return capturedTexture;
}


static Float3 ImportanceSampleGGX(const Float2& e, float a2) {
	float phi = 2 * PI * e.x();
	float cosTheta = sqrtf(Math::Max(0.0f, (1 - e.y()) / Math::Max(EPSILON, 1 + (a2 - 1) * e.y())));
	float sinTheta = sqrtf(Math::Max(0.0f, 1 - cosTheta * cosTheta));

	Float3 h;
	h.x() = sinTheta * cosf(phi);
	h.y() = sinTheta * sinf(phi);
	h.z() = cosTheta;

	return h;
}

static Float3 ImportanceSampleCosWeight(const Float2& e) {
	float phi = 2 * PI * e.x();
	float sinTheta = sqrtf(e.y());
	float cosTheta = sqrtf(Math::Max(0.0f, 1 - e.y()));

	Float3 h;
	h.x() = sinTheta * cosf(phi);
	h.y() = sinTheta * sinf(phi);
	h.z() = cosTheta;

	return h;
}

static Float3 UniformSample(const Float2& e) {
	float phi = 2 * PI * e.x();
	float theta = PI * e.y();
	float sinTheta = sinf(theta);
	float cosTheta = cosf(theta);

	Float3 h;
	h.x() = sinTheta * cosf(phi);
	h.y() = sinTheta * sinf(phi);
	h.z() = cosTheta;

	return h;
}

// https://github.com/headupinclouds/half/blob/master/include/half.hpp
static float half2float_impl(uint16_t value);

static Float4 SampleCubeTexture(const TShared<TextureResource>& texture, const Float3& dir, const Float4& maxEnvironmentRadiance) {
	IRender::Resource::TextureDescription& desc = texture->description;
	assert(desc.state.format == IRender::Resource::TextureDescription::HALF);
	assert(desc.state.layout == IRender::Resource::TextureDescription::RGBA);
	assert(!desc.state.compress);

	// sample cube map
	// order [+x/-x/+y/-y/+z/-z]
	Float3 absDir = Math::Abs(dir);
	uint32_t index;
	Float2 uv;
	if (absDir.y() < absDir.x()) {
		if (absDir.z() < absDir.x()) {
			if (dir.x() >= 0) {
				index = 0;
				uv = Float2(-dir.z(), dir.y()) / absDir.x();
			} else {
				index = 1;
				uv = Float2(dir.z(), dir.y()) / absDir.x();
			}
		} else {
			if (dir.z() >= 0) {
				index = 4;
				uv = Float2(dir.x(), dir.y()) / absDir.z();
			} else {
				index = 5;
				uv = Float2(-dir.x(), dir.y()) / absDir.z();
			}
		}
	} else {
		if (absDir.z() < absDir.y()) {
			if (dir.y() >= 0) {
				index = 3;
				uv = Float2(dir.x(), -dir.z()) / absDir.y();
			} else {
				index = 2;
				uv = Float2(dir.x(), dir.z()) / absDir.y();
			}
		} else {
			if (dir.z() >= 0) {
				index = 4;
				uv = Float2(dir.x(), dir.y()) / absDir.z();
			} else {
				index = 5;
				uv = Float2(-dir.x(), dir.y()) / absDir.z();
			}
		}
	}

	// clamp uv
	uv = uv * 0.5f + 0.5f;
	int ux = Math::Min(desc.dimension.x() - 1, Math::Max(0, (int)(uv.x() * desc.dimension.x() + 0.5f)));
	int uy = Math::Min(desc.dimension.y() - 1, Math::Max(0, (int)(uv.y() * desc.dimension.y() + 0.5f)));

	if (ux < 0) ux += desc.dimension.x();
	if (uy < 0) uy += desc.dimension.y();

	size_t each = desc.data.GetSize() / (6 * sizeof(UShort4));

	UShort4 halfValues = reinterpret_cast<UShort4*>(desc.data.GetData())[uy * desc.dimension.x() + ux + each * index];
	return Math::AllMin(maxEnvironmentRadiance, Float4(half2float_impl(halfValues.x()), half2float_impl(halfValues.y()), half2float_impl(halfValues.z()), 0.0f));
}

static Float4 SampleTexture(const TShared<TextureResource>& texture, const Float2& uv) {
	IRender::Resource::TextureDescription& desc = texture->description;
	assert(desc.state.format == IRender::Resource::TextureDescription::UNSIGNED_BYTE);
	assert(desc.state.layout == IRender::Resource::TextureDescription::RGBA);
	assert(!desc.state.compress);
	// wrap uv
	int ux = (int)(uv.x() * desc.dimension.x() + 0.5f) % desc.dimension.x();
	int uy = (int)(uv.y() * desc.dimension.y() + 0.5f) % desc.dimension.y();

	if (ux < 0) ux += desc.dimension.x();
	if (uy < 0) uy += desc.dimension.y();

	return ToFloat4(reinterpret_cast<UChar4*>(desc.data.GetData())[uy * desc.dimension.x() + ux]);
}

// GGX Geometry
static float Geometry(float a2, float NoL, float NoV) {
	NoL = Math::Clamp(NoL, 0.1f, 1.0f);
	NoV = Math::Clamp(NoV, 0.01f, 1.0f);
	float kl = NoL * sqrtf(Math::Max(0.0f, -NoV * a2 + NoV) * NoV + a2);
	float kv = NoV * sqrtf(Math::Max(0.0f, -NoL * a2 + NoL) * NoL + a2);
	return 0.5f / Math::Max((kl + kv) * PI, EPSILON);
}

// GGX Distribution
static float Distribution(float a2, float NoH) {
	float q = (NoH * a2 - NoH) * NoH + 1.0f;
	return a2 / Math::Max(EPSILON, q * q);
}

static Float4 Fresnel(float a2, float VoH, const Float4& specularColor) {
#if defined(_MSC_VER) && _MSC_VER <= 1200
	float e = expf(VoH * (VoH * -5.55473f) - 6.98316f) / expf(2);
#else
	float e = exp2f(VoH * (VoH * -5.55473f) - 6.98316f);
#endif
	float f = Math::Min(50.0f * specularColor.y(), 0.0f);
	return specularColor + (Float4(f, f, f, f) - specularColor) * e;
}

inline void RayTraceComponent::Raycast(Component::RaycastTaskSerial& task, const TShared<Context>& context, const Float3Pair& r) const {
	const std::vector<SpaceComponent*>& rootSpaceComponents = context->rootSpaceComponents;
	for (size_t i = 0; i < rootSpaceComponents.size(); i++) {
		SpaceComponent* spaceComponent = rootSpaceComponents[i];
		Float3Pair ray = r;
		MatrixFloat4x4 matrix = MatrixFloat4x4::Identity();
		spaceComponent->Raycast(task, ray, matrix, nullptr, 1.0f);
	}
}

Float4 RayTraceComponent::PathTrace(const TShared<Context>& context, const Float3Pair& r, BytesCache& cache, uint32_t count) const {
	RaycastTaskSerial task(&cache);
	Raycast(task, context, r);

	if (task.result.squareDistance != FLT_MAX && task.result.parent) {
		// hit!
		assert(task.result.parent->QueryInterface(UniqueType<Entity>()) != nullptr);
		std::unordered_map<size_t, uint32_t>::const_iterator it = context->mapEntityToResourceIndex.find(reinterpret_cast<size_t>(task.result.parent()));
		if (it != context->mapEntityToResourceIndex.end()) {
			const TShared<TextureResource>& baseColorTexture = static_cast<TextureResource*>(context->mappedResources[(*it).second]());
			const TShared<TextureResource>& normalTexture = static_cast<TextureResource*>(context->mappedResources[(*it).second + 1]());
			const TShared<TextureResource>& mixtureTexture = static_cast<TextureResource*>(context->mappedResources[(*it).second + 2]());
			assert(task.result.unit->QueryInterface(UniqueType<ShapeComponent>()) != nullptr);
			ShapeComponent* shapeComponent = static_cast<ShapeComponent*>(task.result.unit());
			IAsset::MeshCollection& meshCollection = shapeComponent->GetMesh()->meshCollection;

			const UInt3& face = meshCollection.indices[task.result.faceIndex];
			if (!meshCollection.texCoords.empty() && !meshCollection.normals.empty() && !meshCollection.tangents.empty()) {
				Float4 uvBase = meshCollection.texCoords[0].coords[face.x()];
				Float4 uvM = meshCollection.texCoords[0].coords[face.y()];
				Float4 uvN = meshCollection.texCoords[0].coords[face.z()];
				uvBase = uvBase + (uvM - uvBase) * task.result.coord.x() + (uvN - uvBase) * task.result.coord.y();
				Float2 uv(uvBase.x(), uvBase.y());

				// sample texture
				Float4 baseColor = SampleTexture(baseColorTexture, uv);
				for (size_t i = 0; i < 4; i++) {
					baseColor[i] = powf(baseColor[i], 2.2f);
				}

				Float4 normal = SampleTexture(normalTexture, uv) * Float4(2.0f, 2.0f, 2.0f, 2.0f) - Float4(1.0f, 1.0f, 1.0f, 1.0f);
				Float4 mixture = SampleTexture(mixtureTexture, uv);
				float occlusion = mixture.x();
				float roughness = mixture.y();
				float metallic = mixture.z();

				// calc world space normal
				Float4 binormalBase = ToFloat4Signed(meshCollection.normals[face.x()]);
				Float4 binormalM = ToFloat4Signed(meshCollection.normals[face.y()]);
				Float4 binormalN = ToFloat4Signed(meshCollection.normals[face.z()]);

				binormalBase = binormalBase + (binormalM - binormalBase) * task.result.coord.x() + (binormalN - binormalBase) * task.result.coord.y();

				Float4 tangentBase = ToFloat4Signed(meshCollection.tangents[face.x()]);
				Float4 tangentM = ToFloat4Signed(meshCollection.tangents[face.y()]);
				Float4 tangentN = ToFloat4Signed(meshCollection.tangents[face.z()]);

				tangentBase = tangentBase + (tangentM - tangentBase) * task.result.coord.x() + (tangentN - tangentBase) * task.result.coord.y();

				// To world space
				float hand = tangentBase.w();
				tangentBase.w() = binormalBase.w() = 0;
				binormalBase = binormalBase * task.result.transform;
				tangentBase = tangentBase * task.result.transform;
				Float4 normalBase = Math::CrossProduct(binormalBase, tangentBase) * hand;

				Float4 N = normalBase * normal.z() + tangentBase * normal.x() + binormalBase * normal.y();

				N.w() = 0;
				N = Math::Normalize(N);

				// Lighting
				Float4 position(task.result.position.x(), task.result.position.y(), task.result.position.z(), 1.0f);
				position = position * task.result.transform;
				Float3 worldPosition = (Float3)position;
				Float4 radiance(0.0f, 0.0f, 0.0f, 0.0f);

				Float4 V = Float4::Load(r.second);
				V = Math::Normalize(-V);

				float NoV = Math::Max(0.0f, Math::DotProduct(V, N));
				float a = roughness * roughness;
				float a2 = a * a;
				Float4 diffuseColor = (baseColor - baseColor * metallic) / PI;
				Float4 specularColor = Math::Interpolate(Float4(0.04f, 0.04f, 0.04f, 0.04f), baseColor, metallic);

				// russian roulette for tracing geometric lights
				float survivalPossibility = 1;
				
				for (size_t k = 0; k < context->lightElements.size(); k++) {
					const LightElement& lightElement = context->lightElements[k];
					if (lightElement.position.w() == 0) { // directional light only now
						float roll = (float)rand() / RAND_MAX;
						if (roll < context->possibilityForGeometryLight) {
							// check shadow
							Float3 dir = Math::Normalize((Float3)lightElement.position);
							Float4 L = Float4::Load(dir);
							float NoL = Math::Max(0.0f, Math::DotProduct(N, L));

							if (NoL > 0.0f) {
								RaycastTaskSerial task;
								Raycast(task, context, Float3Pair(worldPosition + dir * stepMinimal, dir * stepMaximal));
								if (task.result.squareDistance == FLT_MAX) {
									// not shadowed, compute shading
									Float4 H = Math::Normalize(L + V);
									float NoH = Math::Max(0.0f, Math::DotProduct(N, H));
									float VoH = Math::Max(0.0f, Math::DotProduct(V, H));

									radiance += (diffuseColor + Fresnel(a2, VoH, specularColor) * Distribution(a2, NoH) * Geometry(a2, NoL, NoV)) * lightElement.colorAttenuation * (NoL / (survivalPossibility * context->possibilityForGeometryLight));
								}
							}

							return radiance;
						} else {
							survivalPossibility *= 1.0f - context->possibilityForGeometryLight;
						}
					}
				}

				// gather sky light randomly
				if (context->cubeMapTexture) {
					float roll = (float)rand() / RAND_MAX;
					if (roll < context->possibilityCubeMapLight) {
						Float4 L = context->importantCubeMapDistribution[rand() % context->importantCubeMapDistribution.size()];
						// Float3 h = UniformSample(Float2((float)rand() / RAND_MAX, (float)rand() / RAND_MAX));
						// Float4 L(h.x(), h.y(), h.z(), 1.0f);
						float NoL = Math::Max(0.0f, Math::DotProduct(N, L));

						if (NoL > 0) {
							float pdf = L.w();
							L.w() = 0;

							Float3 dir(L);
							RaycastTaskSerial task(&cache);
							Raycast(task, context, Float3Pair(worldPosition + dir * stepMinimal, dir * stepMaximal));
							if (task.result.squareDistance == FLT_MAX) {
								Float4 r = SampleCubeTexture(context->cubeMapTexture, dir, maxEnvironmentRadiance);
								Float4 H = Math::Normalize(L + V);
								float NoH = Math::Max(0.0f, Math::DotProduct(N, H));
								float VoH = Math::Max(0.0f, Math::DotProduct(V, H));

								radiance += (diffuseColor + Fresnel(a2, VoH, specularColor) * Distribution(a2, NoH) * Geometry(a2, NoL, NoV)) * r / (NoL * survivalPossibility * context->possibilityCubeMapLight * pdf);
							}
						}

						return radiance;
					} else {
						survivalPossibility *= 1.0f - context->possibilityCubeMapLight;
					}
				}

				// PathTrace next
				if (survivalPossibility > 0 && count < maxBounceCount) {
					uint32_t m = count == 0 ? rayCount : 1;
					Float4 gather(0, 0, 0, 0);
					uint32_t validRay = 0;

					for (uint32_t i = 0; i < m; i++) {
						Float2 random = Float2((float)rand() / RAND_MAX, (float)rand() / RAND_MAX);
						float ratio = ((float)rand() / RAND_MAX - 0.04f) / 0.96f;
						bool isSpecular = ratio < metallic;

						Float3 R = isSpecular ? ImportanceSampleGGX(random, a) : ImportanceSampleCosWeight(random);
						Float4 H = Math::Normalize(N * R.z() + Math::Normalize(tangentBase) * R.x() + Math::Normalize(binormalBase) * R.y());
						Float4 L = H * Math::DotProduct(H, V) * 2.0f - V;
						float NoL = Math::DotProduct(N, L);

						if (NoL > EPSILON) {
							Float3Pair ray(worldPosition + (Float3)L * stepMinimal, (Float3)L * stepMaximal);

							if (isSpecular) {
								float NoH = Math::DotProduct(N, H);
								if (NoH > EPSILON) {
									float VoH = Math::Max(0.0f, Math::DotProduct(V, H));
									gather += PathTrace(context, ray, cache, count + 1) * Fresnel(a2, VoH, specularColor) * Geometry(a2, NoL, NoV) * (4.0f * VoH * NoL / NoH);
									validRay++;
								}
							} else {
								gather += PathTrace(context, ray, cache, count + 1) * baseColor;
								validRay++;
							}
						}
					}

					radiance += gather / ((float)Math::Max((uint32_t)1, validRay) * survivalPossibility);
				}

				return radiance;
			}
		}
	}

	if (context->cubeMapTexture) {
		return SampleCubeTexture(context->cubeMapTexture, Math::Normalize(r.second), maxEnvironmentRadiance);
	} else {
		return Float4(0.0f, 0.0f, 0.0f, 0.0f);
	}
}

struct CompareRadiance {
	bool operator () (const Float4& lhs, const Float4& rhs) const {
		return lhs.w() < rhs.w();
	}
};

void RayTraceComponent::RoutineRayTrace(const TShared<Context>& context) {
	RoutineCollectTextures(context, context->referenceCameraComponent->GetBridgeComponent()->GetHostEntity(), MatrixFloat4x4::Identity());
	srand(0);

	if (context->cubeMapTexture) {
		std::vector<Float4> distribution;
		distribution.resize(cubeMapQuantization);
		assert(cubeMapQuantization != 0);
		assert(cubeMapQuantizationMultiplier != 0);

		std::vector<Float4> samples;
		samples.resize(cubeMapQuantization * cubeMapQuantizationMultiplier);

		for (size_t i = 0; i < samples.size(); i++) {
			Float3 h = UniformSample(Float2((float)rand() / RAND_MAX, (float)rand() / RAND_MAX));
			samples[i] = Float4(h.x(), h.y(), h.z(), Math::Length(SampleCubeTexture(context->cubeMapTexture, h, maxEnvironmentRadiance)));
		}

		std::sort(samples.begin(), samples.end(), CompareRadiance());

		// accum sum
		for (size_t j = 1; j < samples.size(); j++) {
			samples[j].w() += samples[j - 1].w();
		}

		float sum = Math::Max(samples.back().w(), EPSILON);
		float lastAccum = 0;
		uint32_t n = 0;
		std::vector<Float4>::iterator lastIterator = samples.end();
		for (uint32_t k = 0; k < cubeMapQuantization; k++) {
			std::vector<Float4>::iterator it = std::lower_bound(samples.begin(), samples.end(), Float4(0, 0, 0, sum * k / cubeMapQuantization), CompareRadiance());
			if (it != lastIterator) {
				Float4 f = *it;
				float w = f.w();
				f.w() = (f.w() - lastAccum) / sum;

				lastAccum = w;
				distribution[n++] = f;
				lastIterator = it;
			}
		}

		context->importantCubeMapDistribution.resize(n);

		for (uint32_t m = 0; m < n; m++) {
			Float4 v = distribution[m];
			v.w() = v.w() * n;
			context->importantCubeMapDistribution[m] = v;
		}
	}

	size_t tileCountWidth = (captureSize.x() + tileSize - 1) / tileSize;
	size_t tileCountHeight = (captureSize.y() + tileSize - 1) / tileSize;

	ThreadPool& threadPool = context->engine.GetKernel().GetThreadPool();
	for (size_t i = 0; i < tileCountHeight; i++) {
		for (size_t j = 0; j < tileCountWidth; j++) {
			threadPool.Dispatch(CreateTaskContextFree(Wrap(this, &RayTraceComponent::RoutineRenderTile), context, j, i), 1);
		}
	}
}

static float half2float_impl(uint16_t value) {
	static const uint32_t mantissa_table[2048] = {
		0x00000000, 0x33800000, 0x34000000, 0x34400000, 0x34800000, 0x34A00000, 0x34C00000, 0x34E00000, 0x35000000, 0x35100000, 0x35200000, 0x35300000, 0x35400000, 0x35500000, 0x35600000, 0x35700000,
		0x35800000, 0x35880000, 0x35900000, 0x35980000, 0x35A00000, 0x35A80000, 0x35B00000, 0x35B80000, 0x35C00000, 0x35C80000, 0x35D00000, 0x35D80000, 0x35E00000, 0x35E80000, 0x35F00000, 0x35F80000,
		0x36000000, 0x36040000, 0x36080000, 0x360C0000, 0x36100000, 0x36140000, 0x36180000, 0x361C0000, 0x36200000, 0x36240000, 0x36280000, 0x362C0000, 0x36300000, 0x36340000, 0x36380000, 0x363C0000,
		0x36400000, 0x36440000, 0x36480000, 0x364C0000, 0x36500000, 0x36540000, 0x36580000, 0x365C0000, 0x36600000, 0x36640000, 0x36680000, 0x366C0000, 0x36700000, 0x36740000, 0x36780000, 0x367C0000,
		0x36800000, 0x36820000, 0x36840000, 0x36860000, 0x36880000, 0x368A0000, 0x368C0000, 0x368E0000, 0x36900000, 0x36920000, 0x36940000, 0x36960000, 0x36980000, 0x369A0000, 0x369C0000, 0x369E0000,
		0x36A00000, 0x36A20000, 0x36A40000, 0x36A60000, 0x36A80000, 0x36AA0000, 0x36AC0000, 0x36AE0000, 0x36B00000, 0x36B20000, 0x36B40000, 0x36B60000, 0x36B80000, 0x36BA0000, 0x36BC0000, 0x36BE0000,
		0x36C00000, 0x36C20000, 0x36C40000, 0x36C60000, 0x36C80000, 0x36CA0000, 0x36CC0000, 0x36CE0000, 0x36D00000, 0x36D20000, 0x36D40000, 0x36D60000, 0x36D80000, 0x36DA0000, 0x36DC0000, 0x36DE0000,
		0x36E00000, 0x36E20000, 0x36E40000, 0x36E60000, 0x36E80000, 0x36EA0000, 0x36EC0000, 0x36EE0000, 0x36F00000, 0x36F20000, 0x36F40000, 0x36F60000, 0x36F80000, 0x36FA0000, 0x36FC0000, 0x36FE0000,
		0x37000000, 0x37010000, 0x37020000, 0x37030000, 0x37040000, 0x37050000, 0x37060000, 0x37070000, 0x37080000, 0x37090000, 0x370A0000, 0x370B0000, 0x370C0000, 0x370D0000, 0x370E0000, 0x370F0000,
		0x37100000, 0x37110000, 0x37120000, 0x37130000, 0x37140000, 0x37150000, 0x37160000, 0x37170000, 0x37180000, 0x37190000, 0x371A0000, 0x371B0000, 0x371C0000, 0x371D0000, 0x371E0000, 0x371F0000,
		0x37200000, 0x37210000, 0x37220000, 0x37230000, 0x37240000, 0x37250000, 0x37260000, 0x37270000, 0x37280000, 0x37290000, 0x372A0000, 0x372B0000, 0x372C0000, 0x372D0000, 0x372E0000, 0x372F0000,
		0x37300000, 0x37310000, 0x37320000, 0x37330000, 0x37340000, 0x37350000, 0x37360000, 0x37370000, 0x37380000, 0x37390000, 0x373A0000, 0x373B0000, 0x373C0000, 0x373D0000, 0x373E0000, 0x373F0000,
		0x37400000, 0x37410000, 0x37420000, 0x37430000, 0x37440000, 0x37450000, 0x37460000, 0x37470000, 0x37480000, 0x37490000, 0x374A0000, 0x374B0000, 0x374C0000, 0x374D0000, 0x374E0000, 0x374F0000,
		0x37500000, 0x37510000, 0x37520000, 0x37530000, 0x37540000, 0x37550000, 0x37560000, 0x37570000, 0x37580000, 0x37590000, 0x375A0000, 0x375B0000, 0x375C0000, 0x375D0000, 0x375E0000, 0x375F0000,
		0x37600000, 0x37610000, 0x37620000, 0x37630000, 0x37640000, 0x37650000, 0x37660000, 0x37670000, 0x37680000, 0x37690000, 0x376A0000, 0x376B0000, 0x376C0000, 0x376D0000, 0x376E0000, 0x376F0000,
		0x37700000, 0x37710000, 0x37720000, 0x37730000, 0x37740000, 0x37750000, 0x37760000, 0x37770000, 0x37780000, 0x37790000, 0x377A0000, 0x377B0000, 0x377C0000, 0x377D0000, 0x377E0000, 0x377F0000,
		0x37800000, 0x37808000, 0x37810000, 0x37818000, 0x37820000, 0x37828000, 0x37830000, 0x37838000, 0x37840000, 0x37848000, 0x37850000, 0x37858000, 0x37860000, 0x37868000, 0x37870000, 0x37878000,
		0x37880000, 0x37888000, 0x37890000, 0x37898000, 0x378A0000, 0x378A8000, 0x378B0000, 0x378B8000, 0x378C0000, 0x378C8000, 0x378D0000, 0x378D8000, 0x378E0000, 0x378E8000, 0x378F0000, 0x378F8000,
		0x37900000, 0x37908000, 0x37910000, 0x37918000, 0x37920000, 0x37928000, 0x37930000, 0x37938000, 0x37940000, 0x37948000, 0x37950000, 0x37958000, 0x37960000, 0x37968000, 0x37970000, 0x37978000,
		0x37980000, 0x37988000, 0x37990000, 0x37998000, 0x379A0000, 0x379A8000, 0x379B0000, 0x379B8000, 0x379C0000, 0x379C8000, 0x379D0000, 0x379D8000, 0x379E0000, 0x379E8000, 0x379F0000, 0x379F8000,
		0x37A00000, 0x37A08000, 0x37A10000, 0x37A18000, 0x37A20000, 0x37A28000, 0x37A30000, 0x37A38000, 0x37A40000, 0x37A48000, 0x37A50000, 0x37A58000, 0x37A60000, 0x37A68000, 0x37A70000, 0x37A78000,
		0x37A80000, 0x37A88000, 0x37A90000, 0x37A98000, 0x37AA0000, 0x37AA8000, 0x37AB0000, 0x37AB8000, 0x37AC0000, 0x37AC8000, 0x37AD0000, 0x37AD8000, 0x37AE0000, 0x37AE8000, 0x37AF0000, 0x37AF8000,
		0x37B00000, 0x37B08000, 0x37B10000, 0x37B18000, 0x37B20000, 0x37B28000, 0x37B30000, 0x37B38000, 0x37B40000, 0x37B48000, 0x37B50000, 0x37B58000, 0x37B60000, 0x37B68000, 0x37B70000, 0x37B78000,
		0x37B80000, 0x37B88000, 0x37B90000, 0x37B98000, 0x37BA0000, 0x37BA8000, 0x37BB0000, 0x37BB8000, 0x37BC0000, 0x37BC8000, 0x37BD0000, 0x37BD8000, 0x37BE0000, 0x37BE8000, 0x37BF0000, 0x37BF8000,
		0x37C00000, 0x37C08000, 0x37C10000, 0x37C18000, 0x37C20000, 0x37C28000, 0x37C30000, 0x37C38000, 0x37C40000, 0x37C48000, 0x37C50000, 0x37C58000, 0x37C60000, 0x37C68000, 0x37C70000, 0x37C78000,
		0x37C80000, 0x37C88000, 0x37C90000, 0x37C98000, 0x37CA0000, 0x37CA8000, 0x37CB0000, 0x37CB8000, 0x37CC0000, 0x37CC8000, 0x37CD0000, 0x37CD8000, 0x37CE0000, 0x37CE8000, 0x37CF0000, 0x37CF8000,
		0x37D00000, 0x37D08000, 0x37D10000, 0x37D18000, 0x37D20000, 0x37D28000, 0x37D30000, 0x37D38000, 0x37D40000, 0x37D48000, 0x37D50000, 0x37D58000, 0x37D60000, 0x37D68000, 0x37D70000, 0x37D78000,
		0x37D80000, 0x37D88000, 0x37D90000, 0x37D98000, 0x37DA0000, 0x37DA8000, 0x37DB0000, 0x37DB8000, 0x37DC0000, 0x37DC8000, 0x37DD0000, 0x37DD8000, 0x37DE0000, 0x37DE8000, 0x37DF0000, 0x37DF8000,
		0x37E00000, 0x37E08000, 0x37E10000, 0x37E18000, 0x37E20000, 0x37E28000, 0x37E30000, 0x37E38000, 0x37E40000, 0x37E48000, 0x37E50000, 0x37E58000, 0x37E60000, 0x37E68000, 0x37E70000, 0x37E78000,
		0x37E80000, 0x37E88000, 0x37E90000, 0x37E98000, 0x37EA0000, 0x37EA8000, 0x37EB0000, 0x37EB8000, 0x37EC0000, 0x37EC8000, 0x37ED0000, 0x37ED8000, 0x37EE0000, 0x37EE8000, 0x37EF0000, 0x37EF8000,
		0x37F00000, 0x37F08000, 0x37F10000, 0x37F18000, 0x37F20000, 0x37F28000, 0x37F30000, 0x37F38000, 0x37F40000, 0x37F48000, 0x37F50000, 0x37F58000, 0x37F60000, 0x37F68000, 0x37F70000, 0x37F78000,
		0x37F80000, 0x37F88000, 0x37F90000, 0x37F98000, 0x37FA0000, 0x37FA8000, 0x37FB0000, 0x37FB8000, 0x37FC0000, 0x37FC8000, 0x37FD0000, 0x37FD8000, 0x37FE0000, 0x37FE8000, 0x37FF0000, 0x37FF8000,
		0x38000000, 0x38004000, 0x38008000, 0x3800C000, 0x38010000, 0x38014000, 0x38018000, 0x3801C000, 0x38020000, 0x38024000, 0x38028000, 0x3802C000, 0x38030000, 0x38034000, 0x38038000, 0x3803C000,
		0x38040000, 0x38044000, 0x38048000, 0x3804C000, 0x38050000, 0x38054000, 0x38058000, 0x3805C000, 0x38060000, 0x38064000, 0x38068000, 0x3806C000, 0x38070000, 0x38074000, 0x38078000, 0x3807C000,
		0x38080000, 0x38084000, 0x38088000, 0x3808C000, 0x38090000, 0x38094000, 0x38098000, 0x3809C000, 0x380A0000, 0x380A4000, 0x380A8000, 0x380AC000, 0x380B0000, 0x380B4000, 0x380B8000, 0x380BC000,
		0x380C0000, 0x380C4000, 0x380C8000, 0x380CC000, 0x380D0000, 0x380D4000, 0x380D8000, 0x380DC000, 0x380E0000, 0x380E4000, 0x380E8000, 0x380EC000, 0x380F0000, 0x380F4000, 0x380F8000, 0x380FC000,
		0x38100000, 0x38104000, 0x38108000, 0x3810C000, 0x38110000, 0x38114000, 0x38118000, 0x3811C000, 0x38120000, 0x38124000, 0x38128000, 0x3812C000, 0x38130000, 0x38134000, 0x38138000, 0x3813C000,
		0x38140000, 0x38144000, 0x38148000, 0x3814C000, 0x38150000, 0x38154000, 0x38158000, 0x3815C000, 0x38160000, 0x38164000, 0x38168000, 0x3816C000, 0x38170000, 0x38174000, 0x38178000, 0x3817C000,
		0x38180000, 0x38184000, 0x38188000, 0x3818C000, 0x38190000, 0x38194000, 0x38198000, 0x3819C000, 0x381A0000, 0x381A4000, 0x381A8000, 0x381AC000, 0x381B0000, 0x381B4000, 0x381B8000, 0x381BC000,
		0x381C0000, 0x381C4000, 0x381C8000, 0x381CC000, 0x381D0000, 0x381D4000, 0x381D8000, 0x381DC000, 0x381E0000, 0x381E4000, 0x381E8000, 0x381EC000, 0x381F0000, 0x381F4000, 0x381F8000, 0x381FC000,
		0x38200000, 0x38204000, 0x38208000, 0x3820C000, 0x38210000, 0x38214000, 0x38218000, 0x3821C000, 0x38220000, 0x38224000, 0x38228000, 0x3822C000, 0x38230000, 0x38234000, 0x38238000, 0x3823C000,
		0x38240000, 0x38244000, 0x38248000, 0x3824C000, 0x38250000, 0x38254000, 0x38258000, 0x3825C000, 0x38260000, 0x38264000, 0x38268000, 0x3826C000, 0x38270000, 0x38274000, 0x38278000, 0x3827C000,
		0x38280000, 0x38284000, 0x38288000, 0x3828C000, 0x38290000, 0x38294000, 0x38298000, 0x3829C000, 0x382A0000, 0x382A4000, 0x382A8000, 0x382AC000, 0x382B0000, 0x382B4000, 0x382B8000, 0x382BC000,
		0x382C0000, 0x382C4000, 0x382C8000, 0x382CC000, 0x382D0000, 0x382D4000, 0x382D8000, 0x382DC000, 0x382E0000, 0x382E4000, 0x382E8000, 0x382EC000, 0x382F0000, 0x382F4000, 0x382F8000, 0x382FC000,
		0x38300000, 0x38304000, 0x38308000, 0x3830C000, 0x38310000, 0x38314000, 0x38318000, 0x3831C000, 0x38320000, 0x38324000, 0x38328000, 0x3832C000, 0x38330000, 0x38334000, 0x38338000, 0x3833C000,
		0x38340000, 0x38344000, 0x38348000, 0x3834C000, 0x38350000, 0x38354000, 0x38358000, 0x3835C000, 0x38360000, 0x38364000, 0x38368000, 0x3836C000, 0x38370000, 0x38374000, 0x38378000, 0x3837C000,
		0x38380000, 0x38384000, 0x38388000, 0x3838C000, 0x38390000, 0x38394000, 0x38398000, 0x3839C000, 0x383A0000, 0x383A4000, 0x383A8000, 0x383AC000, 0x383B0000, 0x383B4000, 0x383B8000, 0x383BC000,
		0x383C0000, 0x383C4000, 0x383C8000, 0x383CC000, 0x383D0000, 0x383D4000, 0x383D8000, 0x383DC000, 0x383E0000, 0x383E4000, 0x383E8000, 0x383EC000, 0x383F0000, 0x383F4000, 0x383F8000, 0x383FC000,
		0x38400000, 0x38404000, 0x38408000, 0x3840C000, 0x38410000, 0x38414000, 0x38418000, 0x3841C000, 0x38420000, 0x38424000, 0x38428000, 0x3842C000, 0x38430000, 0x38434000, 0x38438000, 0x3843C000,
		0x38440000, 0x38444000, 0x38448000, 0x3844C000, 0x38450000, 0x38454000, 0x38458000, 0x3845C000, 0x38460000, 0x38464000, 0x38468000, 0x3846C000, 0x38470000, 0x38474000, 0x38478000, 0x3847C000,
		0x38480000, 0x38484000, 0x38488000, 0x3848C000, 0x38490000, 0x38494000, 0x38498000, 0x3849C000, 0x384A0000, 0x384A4000, 0x384A8000, 0x384AC000, 0x384B0000, 0x384B4000, 0x384B8000, 0x384BC000,
		0x384C0000, 0x384C4000, 0x384C8000, 0x384CC000, 0x384D0000, 0x384D4000, 0x384D8000, 0x384DC000, 0x384E0000, 0x384E4000, 0x384E8000, 0x384EC000, 0x384F0000, 0x384F4000, 0x384F8000, 0x384FC000,
		0x38500000, 0x38504000, 0x38508000, 0x3850C000, 0x38510000, 0x38514000, 0x38518000, 0x3851C000, 0x38520000, 0x38524000, 0x38528000, 0x3852C000, 0x38530000, 0x38534000, 0x38538000, 0x3853C000,
		0x38540000, 0x38544000, 0x38548000, 0x3854C000, 0x38550000, 0x38554000, 0x38558000, 0x3855C000, 0x38560000, 0x38564000, 0x38568000, 0x3856C000, 0x38570000, 0x38574000, 0x38578000, 0x3857C000,
		0x38580000, 0x38584000, 0x38588000, 0x3858C000, 0x38590000, 0x38594000, 0x38598000, 0x3859C000, 0x385A0000, 0x385A4000, 0x385A8000, 0x385AC000, 0x385B0000, 0x385B4000, 0x385B8000, 0x385BC000,
		0x385C0000, 0x385C4000, 0x385C8000, 0x385CC000, 0x385D0000, 0x385D4000, 0x385D8000, 0x385DC000, 0x385E0000, 0x385E4000, 0x385E8000, 0x385EC000, 0x385F0000, 0x385F4000, 0x385F8000, 0x385FC000,
		0x38600000, 0x38604000, 0x38608000, 0x3860C000, 0x38610000, 0x38614000, 0x38618000, 0x3861C000, 0x38620000, 0x38624000, 0x38628000, 0x3862C000, 0x38630000, 0x38634000, 0x38638000, 0x3863C000,
		0x38640000, 0x38644000, 0x38648000, 0x3864C000, 0x38650000, 0x38654000, 0x38658000, 0x3865C000, 0x38660000, 0x38664000, 0x38668000, 0x3866C000, 0x38670000, 0x38674000, 0x38678000, 0x3867C000,
		0x38680000, 0x38684000, 0x38688000, 0x3868C000, 0x38690000, 0x38694000, 0x38698000, 0x3869C000, 0x386A0000, 0x386A4000, 0x386A8000, 0x386AC000, 0x386B0000, 0x386B4000, 0x386B8000, 0x386BC000,
		0x386C0000, 0x386C4000, 0x386C8000, 0x386CC000, 0x386D0000, 0x386D4000, 0x386D8000, 0x386DC000, 0x386E0000, 0x386E4000, 0x386E8000, 0x386EC000, 0x386F0000, 0x386F4000, 0x386F8000, 0x386FC000,
		0x38700000, 0x38704000, 0x38708000, 0x3870C000, 0x38710000, 0x38714000, 0x38718000, 0x3871C000, 0x38720000, 0x38724000, 0x38728000, 0x3872C000, 0x38730000, 0x38734000, 0x38738000, 0x3873C000,
		0x38740000, 0x38744000, 0x38748000, 0x3874C000, 0x38750000, 0x38754000, 0x38758000, 0x3875C000, 0x38760000, 0x38764000, 0x38768000, 0x3876C000, 0x38770000, 0x38774000, 0x38778000, 0x3877C000,
		0x38780000, 0x38784000, 0x38788000, 0x3878C000, 0x38790000, 0x38794000, 0x38798000, 0x3879C000, 0x387A0000, 0x387A4000, 0x387A8000, 0x387AC000, 0x387B0000, 0x387B4000, 0x387B8000, 0x387BC000,
		0x387C0000, 0x387C4000, 0x387C8000, 0x387CC000, 0x387D0000, 0x387D4000, 0x387D8000, 0x387DC000, 0x387E0000, 0x387E4000, 0x387E8000, 0x387EC000, 0x387F0000, 0x387F4000, 0x387F8000, 0x387FC000,
		0x38000000, 0x38002000, 0x38004000, 0x38006000, 0x38008000, 0x3800A000, 0x3800C000, 0x3800E000, 0x38010000, 0x38012000, 0x38014000, 0x38016000, 0x38018000, 0x3801A000, 0x3801C000, 0x3801E000,
		0x38020000, 0x38022000, 0x38024000, 0x38026000, 0x38028000, 0x3802A000, 0x3802C000, 0x3802E000, 0x38030000, 0x38032000, 0x38034000, 0x38036000, 0x38038000, 0x3803A000, 0x3803C000, 0x3803E000,
		0x38040000, 0x38042000, 0x38044000, 0x38046000, 0x38048000, 0x3804A000, 0x3804C000, 0x3804E000, 0x38050000, 0x38052000, 0x38054000, 0x38056000, 0x38058000, 0x3805A000, 0x3805C000, 0x3805E000,
		0x38060000, 0x38062000, 0x38064000, 0x38066000, 0x38068000, 0x3806A000, 0x3806C000, 0x3806E000, 0x38070000, 0x38072000, 0x38074000, 0x38076000, 0x38078000, 0x3807A000, 0x3807C000, 0x3807E000,
		0x38080000, 0x38082000, 0x38084000, 0x38086000, 0x38088000, 0x3808A000, 0x3808C000, 0x3808E000, 0x38090000, 0x38092000, 0x38094000, 0x38096000, 0x38098000, 0x3809A000, 0x3809C000, 0x3809E000,
		0x380A0000, 0x380A2000, 0x380A4000, 0x380A6000, 0x380A8000, 0x380AA000, 0x380AC000, 0x380AE000, 0x380B0000, 0x380B2000, 0x380B4000, 0x380B6000, 0x380B8000, 0x380BA000, 0x380BC000, 0x380BE000,
		0x380C0000, 0x380C2000, 0x380C4000, 0x380C6000, 0x380C8000, 0x380CA000, 0x380CC000, 0x380CE000, 0x380D0000, 0x380D2000, 0x380D4000, 0x380D6000, 0x380D8000, 0x380DA000, 0x380DC000, 0x380DE000,
		0x380E0000, 0x380E2000, 0x380E4000, 0x380E6000, 0x380E8000, 0x380EA000, 0x380EC000, 0x380EE000, 0x380F0000, 0x380F2000, 0x380F4000, 0x380F6000, 0x380F8000, 0x380FA000, 0x380FC000, 0x380FE000,
		0x38100000, 0x38102000, 0x38104000, 0x38106000, 0x38108000, 0x3810A000, 0x3810C000, 0x3810E000, 0x38110000, 0x38112000, 0x38114000, 0x38116000, 0x38118000, 0x3811A000, 0x3811C000, 0x3811E000,
		0x38120000, 0x38122000, 0x38124000, 0x38126000, 0x38128000, 0x3812A000, 0x3812C000, 0x3812E000, 0x38130000, 0x38132000, 0x38134000, 0x38136000, 0x38138000, 0x3813A000, 0x3813C000, 0x3813E000,
		0x38140000, 0x38142000, 0x38144000, 0x38146000, 0x38148000, 0x3814A000, 0x3814C000, 0x3814E000, 0x38150000, 0x38152000, 0x38154000, 0x38156000, 0x38158000, 0x3815A000, 0x3815C000, 0x3815E000,
		0x38160000, 0x38162000, 0x38164000, 0x38166000, 0x38168000, 0x3816A000, 0x3816C000, 0x3816E000, 0x38170000, 0x38172000, 0x38174000, 0x38176000, 0x38178000, 0x3817A000, 0x3817C000, 0x3817E000,
		0x38180000, 0x38182000, 0x38184000, 0x38186000, 0x38188000, 0x3818A000, 0x3818C000, 0x3818E000, 0x38190000, 0x38192000, 0x38194000, 0x38196000, 0x38198000, 0x3819A000, 0x3819C000, 0x3819E000,
		0x381A0000, 0x381A2000, 0x381A4000, 0x381A6000, 0x381A8000, 0x381AA000, 0x381AC000, 0x381AE000, 0x381B0000, 0x381B2000, 0x381B4000, 0x381B6000, 0x381B8000, 0x381BA000, 0x381BC000, 0x381BE000,
		0x381C0000, 0x381C2000, 0x381C4000, 0x381C6000, 0x381C8000, 0x381CA000, 0x381CC000, 0x381CE000, 0x381D0000, 0x381D2000, 0x381D4000, 0x381D6000, 0x381D8000, 0x381DA000, 0x381DC000, 0x381DE000,
		0x381E0000, 0x381E2000, 0x381E4000, 0x381E6000, 0x381E8000, 0x381EA000, 0x381EC000, 0x381EE000, 0x381F0000, 0x381F2000, 0x381F4000, 0x381F6000, 0x381F8000, 0x381FA000, 0x381FC000, 0x381FE000,
		0x38200000, 0x38202000, 0x38204000, 0x38206000, 0x38208000, 0x3820A000, 0x3820C000, 0x3820E000, 0x38210000, 0x38212000, 0x38214000, 0x38216000, 0x38218000, 0x3821A000, 0x3821C000, 0x3821E000,
		0x38220000, 0x38222000, 0x38224000, 0x38226000, 0x38228000, 0x3822A000, 0x3822C000, 0x3822E000, 0x38230000, 0x38232000, 0x38234000, 0x38236000, 0x38238000, 0x3823A000, 0x3823C000, 0x3823E000,
		0x38240000, 0x38242000, 0x38244000, 0x38246000, 0x38248000, 0x3824A000, 0x3824C000, 0x3824E000, 0x38250000, 0x38252000, 0x38254000, 0x38256000, 0x38258000, 0x3825A000, 0x3825C000, 0x3825E000,
		0x38260000, 0x38262000, 0x38264000, 0x38266000, 0x38268000, 0x3826A000, 0x3826C000, 0x3826E000, 0x38270000, 0x38272000, 0x38274000, 0x38276000, 0x38278000, 0x3827A000, 0x3827C000, 0x3827E000,
		0x38280000, 0x38282000, 0x38284000, 0x38286000, 0x38288000, 0x3828A000, 0x3828C000, 0x3828E000, 0x38290000, 0x38292000, 0x38294000, 0x38296000, 0x38298000, 0x3829A000, 0x3829C000, 0x3829E000,
		0x382A0000, 0x382A2000, 0x382A4000, 0x382A6000, 0x382A8000, 0x382AA000, 0x382AC000, 0x382AE000, 0x382B0000, 0x382B2000, 0x382B4000, 0x382B6000, 0x382B8000, 0x382BA000, 0x382BC000, 0x382BE000,
		0x382C0000, 0x382C2000, 0x382C4000, 0x382C6000, 0x382C8000, 0x382CA000, 0x382CC000, 0x382CE000, 0x382D0000, 0x382D2000, 0x382D4000, 0x382D6000, 0x382D8000, 0x382DA000, 0x382DC000, 0x382DE000,
		0x382E0000, 0x382E2000, 0x382E4000, 0x382E6000, 0x382E8000, 0x382EA000, 0x382EC000, 0x382EE000, 0x382F0000, 0x382F2000, 0x382F4000, 0x382F6000, 0x382F8000, 0x382FA000, 0x382FC000, 0x382FE000,
		0x38300000, 0x38302000, 0x38304000, 0x38306000, 0x38308000, 0x3830A000, 0x3830C000, 0x3830E000, 0x38310000, 0x38312000, 0x38314000, 0x38316000, 0x38318000, 0x3831A000, 0x3831C000, 0x3831E000,
		0x38320000, 0x38322000, 0x38324000, 0x38326000, 0x38328000, 0x3832A000, 0x3832C000, 0x3832E000, 0x38330000, 0x38332000, 0x38334000, 0x38336000, 0x38338000, 0x3833A000, 0x3833C000, 0x3833E000,
		0x38340000, 0x38342000, 0x38344000, 0x38346000, 0x38348000, 0x3834A000, 0x3834C000, 0x3834E000, 0x38350000, 0x38352000, 0x38354000, 0x38356000, 0x38358000, 0x3835A000, 0x3835C000, 0x3835E000,
		0x38360000, 0x38362000, 0x38364000, 0x38366000, 0x38368000, 0x3836A000, 0x3836C000, 0x3836E000, 0x38370000, 0x38372000, 0x38374000, 0x38376000, 0x38378000, 0x3837A000, 0x3837C000, 0x3837E000,
		0x38380000, 0x38382000, 0x38384000, 0x38386000, 0x38388000, 0x3838A000, 0x3838C000, 0x3838E000, 0x38390000, 0x38392000, 0x38394000, 0x38396000, 0x38398000, 0x3839A000, 0x3839C000, 0x3839E000,
		0x383A0000, 0x383A2000, 0x383A4000, 0x383A6000, 0x383A8000, 0x383AA000, 0x383AC000, 0x383AE000, 0x383B0000, 0x383B2000, 0x383B4000, 0x383B6000, 0x383B8000, 0x383BA000, 0x383BC000, 0x383BE000,
		0x383C0000, 0x383C2000, 0x383C4000, 0x383C6000, 0x383C8000, 0x383CA000, 0x383CC000, 0x383CE000, 0x383D0000, 0x383D2000, 0x383D4000, 0x383D6000, 0x383D8000, 0x383DA000, 0x383DC000, 0x383DE000,
		0x383E0000, 0x383E2000, 0x383E4000, 0x383E6000, 0x383E8000, 0x383EA000, 0x383EC000, 0x383EE000, 0x383F0000, 0x383F2000, 0x383F4000, 0x383F6000, 0x383F8000, 0x383FA000, 0x383FC000, 0x383FE000,
		0x38400000, 0x38402000, 0x38404000, 0x38406000, 0x38408000, 0x3840A000, 0x3840C000, 0x3840E000, 0x38410000, 0x38412000, 0x38414000, 0x38416000, 0x38418000, 0x3841A000, 0x3841C000, 0x3841E000,
		0x38420000, 0x38422000, 0x38424000, 0x38426000, 0x38428000, 0x3842A000, 0x3842C000, 0x3842E000, 0x38430000, 0x38432000, 0x38434000, 0x38436000, 0x38438000, 0x3843A000, 0x3843C000, 0x3843E000,
		0x38440000, 0x38442000, 0x38444000, 0x38446000, 0x38448000, 0x3844A000, 0x3844C000, 0x3844E000, 0x38450000, 0x38452000, 0x38454000, 0x38456000, 0x38458000, 0x3845A000, 0x3845C000, 0x3845E000,
		0x38460000, 0x38462000, 0x38464000, 0x38466000, 0x38468000, 0x3846A000, 0x3846C000, 0x3846E000, 0x38470000, 0x38472000, 0x38474000, 0x38476000, 0x38478000, 0x3847A000, 0x3847C000, 0x3847E000,
		0x38480000, 0x38482000, 0x38484000, 0x38486000, 0x38488000, 0x3848A000, 0x3848C000, 0x3848E000, 0x38490000, 0x38492000, 0x38494000, 0x38496000, 0x38498000, 0x3849A000, 0x3849C000, 0x3849E000,
		0x384A0000, 0x384A2000, 0x384A4000, 0x384A6000, 0x384A8000, 0x384AA000, 0x384AC000, 0x384AE000, 0x384B0000, 0x384B2000, 0x384B4000, 0x384B6000, 0x384B8000, 0x384BA000, 0x384BC000, 0x384BE000,
		0x384C0000, 0x384C2000, 0x384C4000, 0x384C6000, 0x384C8000, 0x384CA000, 0x384CC000, 0x384CE000, 0x384D0000, 0x384D2000, 0x384D4000, 0x384D6000, 0x384D8000, 0x384DA000, 0x384DC000, 0x384DE000,
		0x384E0000, 0x384E2000, 0x384E4000, 0x384E6000, 0x384E8000, 0x384EA000, 0x384EC000, 0x384EE000, 0x384F0000, 0x384F2000, 0x384F4000, 0x384F6000, 0x384F8000, 0x384FA000, 0x384FC000, 0x384FE000,
		0x38500000, 0x38502000, 0x38504000, 0x38506000, 0x38508000, 0x3850A000, 0x3850C000, 0x3850E000, 0x38510000, 0x38512000, 0x38514000, 0x38516000, 0x38518000, 0x3851A000, 0x3851C000, 0x3851E000,
		0x38520000, 0x38522000, 0x38524000, 0x38526000, 0x38528000, 0x3852A000, 0x3852C000, 0x3852E000, 0x38530000, 0x38532000, 0x38534000, 0x38536000, 0x38538000, 0x3853A000, 0x3853C000, 0x3853E000,
		0x38540000, 0x38542000, 0x38544000, 0x38546000, 0x38548000, 0x3854A000, 0x3854C000, 0x3854E000, 0x38550000, 0x38552000, 0x38554000, 0x38556000, 0x38558000, 0x3855A000, 0x3855C000, 0x3855E000,
		0x38560000, 0x38562000, 0x38564000, 0x38566000, 0x38568000, 0x3856A000, 0x3856C000, 0x3856E000, 0x38570000, 0x38572000, 0x38574000, 0x38576000, 0x38578000, 0x3857A000, 0x3857C000, 0x3857E000,
		0x38580000, 0x38582000, 0x38584000, 0x38586000, 0x38588000, 0x3858A000, 0x3858C000, 0x3858E000, 0x38590000, 0x38592000, 0x38594000, 0x38596000, 0x38598000, 0x3859A000, 0x3859C000, 0x3859E000,
		0x385A0000, 0x385A2000, 0x385A4000, 0x385A6000, 0x385A8000, 0x385AA000, 0x385AC000, 0x385AE000, 0x385B0000, 0x385B2000, 0x385B4000, 0x385B6000, 0x385B8000, 0x385BA000, 0x385BC000, 0x385BE000,
		0x385C0000, 0x385C2000, 0x385C4000, 0x385C6000, 0x385C8000, 0x385CA000, 0x385CC000, 0x385CE000, 0x385D0000, 0x385D2000, 0x385D4000, 0x385D6000, 0x385D8000, 0x385DA000, 0x385DC000, 0x385DE000,
		0x385E0000, 0x385E2000, 0x385E4000, 0x385E6000, 0x385E8000, 0x385EA000, 0x385EC000, 0x385EE000, 0x385F0000, 0x385F2000, 0x385F4000, 0x385F6000, 0x385F8000, 0x385FA000, 0x385FC000, 0x385FE000,
		0x38600000, 0x38602000, 0x38604000, 0x38606000, 0x38608000, 0x3860A000, 0x3860C000, 0x3860E000, 0x38610000, 0x38612000, 0x38614000, 0x38616000, 0x38618000, 0x3861A000, 0x3861C000, 0x3861E000,
		0x38620000, 0x38622000, 0x38624000, 0x38626000, 0x38628000, 0x3862A000, 0x3862C000, 0x3862E000, 0x38630000, 0x38632000, 0x38634000, 0x38636000, 0x38638000, 0x3863A000, 0x3863C000, 0x3863E000,
		0x38640000, 0x38642000, 0x38644000, 0x38646000, 0x38648000, 0x3864A000, 0x3864C000, 0x3864E000, 0x38650000, 0x38652000, 0x38654000, 0x38656000, 0x38658000, 0x3865A000, 0x3865C000, 0x3865E000,
		0x38660000, 0x38662000, 0x38664000, 0x38666000, 0x38668000, 0x3866A000, 0x3866C000, 0x3866E000, 0x38670000, 0x38672000, 0x38674000, 0x38676000, 0x38678000, 0x3867A000, 0x3867C000, 0x3867E000,
		0x38680000, 0x38682000, 0x38684000, 0x38686000, 0x38688000, 0x3868A000, 0x3868C000, 0x3868E000, 0x38690000, 0x38692000, 0x38694000, 0x38696000, 0x38698000, 0x3869A000, 0x3869C000, 0x3869E000,
		0x386A0000, 0x386A2000, 0x386A4000, 0x386A6000, 0x386A8000, 0x386AA000, 0x386AC000, 0x386AE000, 0x386B0000, 0x386B2000, 0x386B4000, 0x386B6000, 0x386B8000, 0x386BA000, 0x386BC000, 0x386BE000,
		0x386C0000, 0x386C2000, 0x386C4000, 0x386C6000, 0x386C8000, 0x386CA000, 0x386CC000, 0x386CE000, 0x386D0000, 0x386D2000, 0x386D4000, 0x386D6000, 0x386D8000, 0x386DA000, 0x386DC000, 0x386DE000,
		0x386E0000, 0x386E2000, 0x386E4000, 0x386E6000, 0x386E8000, 0x386EA000, 0x386EC000, 0x386EE000, 0x386F0000, 0x386F2000, 0x386F4000, 0x386F6000, 0x386F8000, 0x386FA000, 0x386FC000, 0x386FE000,
		0x38700000, 0x38702000, 0x38704000, 0x38706000, 0x38708000, 0x3870A000, 0x3870C000, 0x3870E000, 0x38710000, 0x38712000, 0x38714000, 0x38716000, 0x38718000, 0x3871A000, 0x3871C000, 0x3871E000,
		0x38720000, 0x38722000, 0x38724000, 0x38726000, 0x38728000, 0x3872A000, 0x3872C000, 0x3872E000, 0x38730000, 0x38732000, 0x38734000, 0x38736000, 0x38738000, 0x3873A000, 0x3873C000, 0x3873E000,
		0x38740000, 0x38742000, 0x38744000, 0x38746000, 0x38748000, 0x3874A000, 0x3874C000, 0x3874E000, 0x38750000, 0x38752000, 0x38754000, 0x38756000, 0x38758000, 0x3875A000, 0x3875C000, 0x3875E000,
		0x38760000, 0x38762000, 0x38764000, 0x38766000, 0x38768000, 0x3876A000, 0x3876C000, 0x3876E000, 0x38770000, 0x38772000, 0x38774000, 0x38776000, 0x38778000, 0x3877A000, 0x3877C000, 0x3877E000,
		0x38780000, 0x38782000, 0x38784000, 0x38786000, 0x38788000, 0x3878A000, 0x3878C000, 0x3878E000, 0x38790000, 0x38792000, 0x38794000, 0x38796000, 0x38798000, 0x3879A000, 0x3879C000, 0x3879E000,
		0x387A0000, 0x387A2000, 0x387A4000, 0x387A6000, 0x387A8000, 0x387AA000, 0x387AC000, 0x387AE000, 0x387B0000, 0x387B2000, 0x387B4000, 0x387B6000, 0x387B8000, 0x387BA000, 0x387BC000, 0x387BE000,
		0x387C0000, 0x387C2000, 0x387C4000, 0x387C6000, 0x387C8000, 0x387CA000, 0x387CC000, 0x387CE000, 0x387D0000, 0x387D2000, 0x387D4000, 0x387D6000, 0x387D8000, 0x387DA000, 0x387DC000, 0x387DE000,
		0x387E0000, 0x387E2000, 0x387E4000, 0x387E6000, 0x387E8000, 0x387EA000, 0x387EC000, 0x387EE000, 0x387F0000, 0x387F2000, 0x387F4000, 0x387F6000, 0x387F8000, 0x387FA000, 0x387FC000, 0x387FE000 };
	static const uint32_t exponent_table[64] = {
		0x00000000, 0x00800000, 0x01000000, 0x01800000, 0x02000000, 0x02800000, 0x03000000, 0x03800000, 0x04000000, 0x04800000, 0x05000000, 0x05800000, 0x06000000, 0x06800000, 0x07000000, 0x07800000,
		0x08000000, 0x08800000, 0x09000000, 0x09800000, 0x0A000000, 0x0A800000, 0x0B000000, 0x0B800000, 0x0C000000, 0x0C800000, 0x0D000000, 0x0D800000, 0x0E000000, 0x0E800000, 0x0F000000, 0x47800000,
		0x80000000, 0x80800000, 0x81000000, 0x81800000, 0x82000000, 0x82800000, 0x83000000, 0x83800000, 0x84000000, 0x84800000, 0x85000000, 0x85800000, 0x86000000, 0x86800000, 0x87000000, 0x87800000,
		0x88000000, 0x88800000, 0x89000000, 0x89800000, 0x8A000000, 0x8A800000, 0x8B000000, 0x8B800000, 0x8C000000, 0x8C800000, 0x8D000000, 0x8D800000, 0x8E000000, 0x8E800000, 0x8F000000, 0xC7800000 };
	static const unsigned short offset_table[64] = {
		   0, 1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024,
		   0, 1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024 };
	uint32_t bits = mantissa_table[offset_table[value >> 10] + (value & 0x3FF)] + exponent_table[value >> 10];
	//			uint32 bits = mantissa_table[(((value&0x7C00)!=0)<<10)+(value&0x3FF)] + exponent_table[value>>10];
	//			return *reinterpret_cast<float*>(&bits);			//violating strict aliasing!
	float out;
	memcpy(&out, &bits, sizeof(float));
	return out;
}

