#include "RayTraceComponent.h"
#include "../Transform/TransformComponent.h"
#include "../Model/ModelComponent.h"
#include "../Space/SpaceComponent.h"
using namespace PaintsNow;

RayTraceComponent::Context::Context(Engine& e) : engine(e) {
	completedPixelCount.store(0, std::memory_order_relaxed);
}

RayTraceComponent::Context::~Context() {
	for (size_t k = 0; k < mappedResources.size(); k++) {
		mappedResources[k]->UnMap();
	}
}

RayTraceComponent::RayTraceComponent() : captureSize(640, 480), superSample(4), tileSize(8), rayCount(1024) {
}

RayTraceComponent::~RayTraceComponent() {
	Cleanup();
}

void RayTraceComponent::Configure(uint16_t s, uint16_t t, uint32_t r) {
	superSample = s;
	tileSize = t;
	rayCount = r;
}

void RayTraceComponent::SetCaptureSize(const UShort2& size) {
	captureSize = size;
}

const UShort2& RayTraceComponent::GetCaptureSize() const {
	return captureSize;
}

size_t RayTraceComponent::GetCompletedPixelCount() const {
	return currentContext() == nullptr ? 0 : currentContext->completedPixelCount.load(std::memory_order_acquire);
}

void RayTraceComponent::Cleanup() {
	currentContext = nullptr;
}

void RayTraceComponent::Capture(Engine& engine, const TShared<CameraComponent>& cameraComponent) {
	assert(currentContext() == nullptr);
	assert(cameraComponent() != nullptr);

	TShared<Context> context = TShared<Context>::From(new Context(engine));
	currentContext = context;
	context->referenceCameraComponent = cameraComponent;
	CameraComponentConfig::WorldGlobalData& worldGlobalData = cameraComponent->GetTaskData()->worldGlobalData;
	context->view = worldGlobalData.viewPosition;
	MatrixFloat4x4 inverseViewProjectionMatrix = Math::QuickInverse(worldGlobalData.projectionMatrix) * Math::QuickInverse(worldGlobalData.viewMatrix);
	context->right = Math::Transform(inverseViewProjectionMatrix, Float3(1, 0, 0));
	context->up = Math::Transform(inverseViewProjectionMatrix, Float3(0, 1, 0));
	context->forward = Math::Transform(inverseViewProjectionMatrix, Float3(0, 0, 1));

	Entity* hostEntity = context->referenceCameraComponent->GetHostEntity();
	const std::vector<Component*>& components = hostEntity->GetComponents();
	std::vector<SpaceComponent*> spaceComponents;
	for (size_t i = 0; i < components.size(); i++) {
		Component* component = components[i];
		if (component == nullptr) continue;

		if (component->GetEntityFlagMask() & Entity::ENTITY_HAS_SPACE) {
			context->rootSpaceComponents.emplace_back(static_cast<SpaceComponent*>(component));
		}
	}

	engine.GetKernel().GetThreadPool().Dispatch(CreateTaskContextFree(Wrap(this, &RayTraceComponent::RoutineRayTrace), context), 1);
}

void RayTraceComponent::RoutineRayTrace(const TShared<Context>& context) {
	RoutineCollectTextures(context, context->referenceCameraComponent->GetHostEntity(), MatrixFloat4x4::Identity());
	size_t tileCountWidth = (captureSize.x() + tileSize - 1) / tileSize;
	size_t tileCountHeight = (captureSize.y() + tileSize - 1) / tileSize;

	ThreadPool& threadPool = context->engine.GetKernel().GetThreadPool();
	for (size_t i = 0; i < tileCountHeight; i++) {
		for (size_t j = 0; j < tileCountWidth; j++) {
			threadPool.Dispatch(CreateTaskContextFree(Wrap(this, &RayTraceComponent::RoutineRenderTile), context, i, j), 1);
		}
	}
}

void RayTraceComponent::RoutineRenderTile(const TShared<Context>& context, size_t i, size_t j) {
	uint32_t width = Math::Min((uint32_t)tileSize, (uint32_t)(captureSize.x() - i * tileSize));
	uint32_t height = Math::Min((uint32_t)tileSize, (uint32_t)(captureSize.y() - j * tileSize));
	const Float3& view = context->view;
	const std::vector<SpaceComponent*>& rootSpaceComponents = context->rootSpaceComponents;

	for (uint32_t m = 0; m < height; m++) {
		for (uint32_t n = 0; n < width; n++) {
			uint32_t px = i * tileSize + n;
			uint32_t py = j * tileSize + m;

			for (uint32_t t = 0; t < superSample; t++) {
				for (uint32_t r = 0; r < superSample; r++) {
					float x = ((r + 0.5f) / superSample + px) / captureSize.x() * 2.0f - 1.0f;
					float y = ((t + 0.5f) / superSample + py) / captureSize.y() * 2.0f - 1.0f;

					Float3 dir = context->forward + context->right * x + context->up * y;
					Component::RaycastTaskSerial task;
					Float3Pair ray(view, dir);
					for (size_t i = 0; i < rootSpaceComponents.size(); i++) {
						SpaceComponent* spaceComponent = rootSpaceComponents[i];
						spaceComponent->Raycast(task, ray, nullptr, 1.0f);
					}

					if (task.result.distance != FLT_MAX) {
						// hit!
						// TODO: resolve material
					}
				}
			}

			// finish one
			context->completedPixelCount.fetch_add(1, std::memory_order_release);
		}
	}

	if (context->completedPixelCount.load(std::memory_order_acquire) == captureSize.x() * captureSize.y()) {
		// Go finish!
		context->engine.GetKernel().QueueRoutine(this, CreateTaskContextFree(Wrap(this, &RayTraceComponent::RoutineComplete), context));
	}
}

void RayTraceComponent::RoutineComplete(const TShared<Context>& context) {
	// TODO: complete!

	if (!outputPath.empty() && context->capturedTexture() != nullptr) {
		IImage& image = context->engine.interfaces.image;
		IRender::Resource::TextureDescription& description = context->capturedTexture->description;
		IRender::Resource::TextureDescription::Layout layout = (IRender::Resource::TextureDescription::Layout)description.state.layout;
		IRender::Resource::TextureDescription::Format format = (IRender::Resource::TextureDescription::Format)description.state.format;

		uint64_t length;
		IStreamBase* stream = context->engine.interfaces.archive.Open(outputPath, true, length);
		IImage::Image* png = image.Create(description.dimension.x(), description.dimension.y(), layout, format);
		void* buffer = image.GetBuffer(png);
		memcpy(buffer, description.data.GetData(), verify_cast<size_t>(description.data.GetSize()));
		image.Save(png, *stream, "png");
		image.Delete(png);
		// write png
		stream->Destroy();
	}
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
						if (lightComponent->Flag().load(std::memory_order_relaxed) & LightComponent::LIGHTCOMPONENT_DIRECTIONAL) {
							element.position = Float4(-currentTransform(2, 0), -currentTransform(2, 1), -currentTransform(2, 2), 0);
						} else {
							float range = lightComponent->GetRange().x();
							element.position = Float4(currentTransform(3, 0), currentTransform(3, 1), currentTransform(3, 2), Math::Max(0.05f, range * range));
						}

						context->lightElements.emplace_back(std::move(element));
					}
				} else {
					ModelComponent* modelComponent = component->QueryInterface(UniqueType<ModelComponent>());
					if (modelComponent != nullptr) {
						const std::vector<std::pair<uint32_t, TShared<MaterialResource> > >& materials = modelComponent->GetMaterials();
						for (size_t j = 0; j < materials.size(); j++) {
							const TShared<MaterialResource>& material = materials[j].second;
							// assume pbr material
							for (size_t k = 0; k < material->materialParams.variables.size(); k++) {
								const IAsset::Material::Variable& var = material->materialParams.variables[k];
								if (var.key == StaticBytes(baseColorTexture) || var.key == StaticBytes(normalTexture) || var.key == StaticBytes(mixtureTexture)) {
									const TShared<TextureResource>& texture = material->textureResources[var.Parse(UniqueType<IAsset::TextureIndex>()).value];
									context->mappedResources.emplace_back(texture->MapRawTexture()());
								}
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
	return currentContext() == nullptr ? nullptr : currentContext->capturedTexture;
}
