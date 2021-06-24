#include "RayTraceComponent.h"
#include "../Transform/TransformComponent.h"
#include "../Model/ModelComponent.h"
#include "../Space/SpaceComponent.h"
#include "../Shape/ShapeComponent.h"
#include "../../../SnowyStream/Manager/RenderResourceManager.h"
using namespace PaintsNow;

RayTraceComponent::Context::Context(Engine& e) : engine(e) {
	completedPixelCount.store(0, std::memory_order_relaxed);
}

RayTraceComponent::Context::~Context() {
	for (size_t k = 0; k < mappedResources.size(); k++) {
		mappedResources[k]->UnMap();
	}
}

 RayTraceComponent::RayTraceComponent() : captureSize(320, 240), superSample(4), tileSize(8), rayCount(16), maxBounceCount(2), completedPixelCountSync(0), stepMinimal(0.05f), stepMaximal(1000.0f) {}
 // RayTraceComponent::RayTraceComponent() : captureSize(320, 240), superSample(2), tileSize(8), rayCount(16), maxBounceCount(4), completedPixelCountSync(0), stepMinimal(0.05f), stepMaximal(1000.0f) {}

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

void RayTraceComponent::RoutineRayTrace(const TShared<Context>& context) {
	RoutineCollectTextures(context, context->referenceCameraComponent->GetBridgeComponent()->GetHostEntity(), MatrixFloat4x4::Identity());

	size_t tileCountWidth = (captureSize.x() + tileSize - 1) / tileSize;
	size_t tileCountHeight = (captureSize.y() + tileSize - 1) / tileSize;

	ThreadPool& threadPool = context->engine.GetKernel().GetThreadPool();
	for (size_t i = 0; i < tileCountHeight; i++) {
		for (size_t j = 0; j < tileCountWidth; j++) {
			threadPool.Dispatch(CreateTaskContextFree(Wrap(this, &RayTraceComponent::RoutineRenderTile), context, j, i), 1);
		}
	}
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
	float averageLuminance = context->invAverageLuminance;
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
			ptr[py * captureSize.x() + px] = ToneMapping(finalColor, averageLuminance); // assuming average color

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
	float cosTheta = sqrtf(Math::Max(0.0f, (1 - e.y()) / Math::Max(0.00001f, 1 + (a2 - 1) * e.y())));
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

static Float4 Render(float a2, float NoH, float NoL, float NoV, float VoH, const Float4& specularColor, const Float4& diffuseColor, const Float4& lightColor) {
	float q = (NoH * a2 - NoH) * NoH + 1.0f;
	float vl = Math::Clamp(NoL, 0.1f, 1.0f);
	float vlc = Math::Clamp(NoV, 0.01f, 1.0f);
	float vls = vl * sqrtf(Math::Max(0.0f, -vlc * a2 + vlc) * vlc + a2);
	vls = vls + sqrtf(Math::Max(0.0f, -vl * a2 + vl) * vl + a2) * vlc;
	float DG = 0.5f * a2 / PI / Math::Max(vls * q * q, 0.00001f);
#if defined(_MSC_VER) && _MSC_VER <= 1200
	float e = expf(VoH * (VoH * -5.55473f) - 6.98316f) / expf(2);
#else
	float e = exp2f(VoH * (VoH * -5.55473f) - 6.98316f);
#endif
	float f = Math::Min(50.0f * specularColor.y(), 0.0f);
	Float4 F = specularColor + (Float4(f, f, f, f) - specularColor) * e;

	return (diffuseColor + F * DG) * lightColor * NoL;
}

Float4 RayTraceComponent::PathTrace(const TShared<Context>& context, const Float3Pair& r, BytesCache& cache, uint32_t count) const {
	const std::vector<SpaceComponent*>& rootSpaceComponents = context->rootSpaceComponents;
	Component::RaycastTaskSerial task(&cache);
	for (size_t i = 0; i < rootSpaceComponents.size(); i++) {
		SpaceComponent* spaceComponent = rootSpaceComponents[i];
		Float3Pair ray = r;
		MatrixFloat4x4 matrix = MatrixFloat4x4::Identity();
		spaceComponent->Raycast(task, ray, matrix, nullptr, 1.0f);
	}

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

				for (size_t k = 0; k < context->lightElements.size(); k++) {
					const LightElement& lightElement = context->lightElements[k];
					if (lightElement.position.w() == 0) { // directional light only now
						// check shadow
						Float3 dir = Math::Normalize((Float3)lightElement.position);
						Float4 L = Float4::Load(dir);
						float NoL = Math::Max(0.0f, Math::DotProduct(N, L));

						if (NoL > 0.0f) {
							Component::RaycastTaskSerial task;
							for (size_t n = 0; n < rootSpaceComponents.size(); n++) {
								SpaceComponent* spaceComponent = rootSpaceComponents[n];
								Float3Pair ray(worldPosition + dir * stepMinimal, dir * stepMaximal);
								MatrixFloat4x4 matrix = MatrixFloat4x4::Identity();
								spaceComponent->Raycast(task, ray, matrix, nullptr, 1.0f);
							}

							if (task.result.squareDistance == FLT_MAX) {
								// not shadowed, compute shading
								Float4 H = Math::Normalize(L + V);
								float NoH = Math::Max(0.0f, Math::DotProduct(N, H));
								float VoH = Math::Max(0.0f, Math::DotProduct(V, H));

								radiance += Render(a2, NoH, NoL, NoV, VoH, specularColor, diffuseColor, lightElement.colorAttenuation);
							}
						}
					}
				}

				// PathTrace next
				if (count < maxBounceCount) {
					uint32_t m = count == 0 ? rayCount : 1;

					for (uint32_t i = 0; i < m; i++) {
						Float2 random = Float2((float)rand() / RAND_MAX, (float)rand() / RAND_MAX);
						float ratio = ((float)rand() / RAND_MAX - 0.04f) / 0.96f;
						bool isSpecular = ratio < metallic;

						Float3 R = isSpecular ? ImportanceSampleGGX(random, a) : ImportanceSampleCosWeight(random);
						Float4 H = Math::Normalize(N * R.z() + Math::Normalize(tangentBase) * R.x() + Math::Normalize(binormalBase) * R.y());
						Float4 L = H * Math::DotProduct(H, V) * 2.0f - V;
						float NoL = Math::Max(0.0f, Math::DotProduct(N, L));
						Float3Pair ray(worldPosition + (Float3)L * stepMinimal, (Float3)L * stepMaximal);
						Float4 bounceRadiance = PathTrace(context, ray, cache, count + 1);
						float VoH = Math::Max(0.0f, Math::DotProduct(V, H));
						float NoH = Math::Max(0.0f, Math::DotProduct(N, H));

						Float4 result = isSpecular ? Render(a2, NoH, NoL, NoV, VoH, specularColor, Float4(0, 0, 0, 0), bounceRadiance) : bounceRadiance * diffuseColor * NoL;

						float q = (NoH * a2 - NoH) * NoH + 1.0f;
						float weight = Math::Max(0.001f, Math::Interpolate(a2 / Math::Max(q * q, 0.0001f) * NoH / Math::Max(0.0001f, 4.0f * VoH), NoL / PI, ratio));

						radiance += result / weight;
					}
				}

				return radiance;
			}
		}
	}

	// TODO: test light
	return Float4(0, 0, 0, 0);
}
