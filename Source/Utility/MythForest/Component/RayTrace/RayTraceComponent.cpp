#include "RayTraceComponent.h"
#include "../Transform/TransformComponent.h"
#include "../Model/ModelComponent.h"
using namespace PaintsNow;

RayTraceComponent::Context::Context(Engine& e) : engine(e) {
	completedPixelCount.store(0, std::memory_order_relaxed);
}

RayTraceComponent::Context::~Context() {
	for (size_t k = 0; k < mappedResources.size(); k++) {
		mappedResources[k]->UnMap();
	}
}

RayTraceComponent::RayTraceComponent() : captureSize(640, 480), superSample(1024), tileSize(8) {
}

RayTraceComponent::~RayTraceComponent() {
	Cleanup();
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
	// TODO: render tile
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
