#include "TextureArrayResource.h"
#include "../Manager/RenderResourceManager.h"
using namespace PaintsNow;

TextureArrayResource::TextureArrayResource(ResourceManager& manager, const String& uniqueID) : BaseClass(manager, uniqueID) {}

void TextureArrayResource::Refresh(IRender& device, void* deviceContext) {}
void TextureArrayResource::Download(IRender& device, void* deviceContext) {}
void TextureArrayResource::Upload(IRender& device, void* deviceContext) {}
void TextureArrayResource::Attach(IRender& device, void* deviceContext) {}
void TextureArrayResource::Detach(IRender& device, void* deviceContext) {}

TObject<IReflect>& TextureArrayResource::operator () (IReflect& reflect) {
	BaseClass::operator () (reflect);

	if (reflect.IsReflectProperty()) {
		ReflectProperty(slices)[MetaResourceInternalPersist(resourceManager)];
	}

	return *this;
}

bool TextureArrayResource::Map() {
	if (!ResourceBase::Map()) return false;

	bool allDone = false;

	for (size_t i = 0; i < slices.size(); i++) {
		const TShared<TextureResource>& texture = slices[i];
		allDone = (texture->Map() && (i == 0 || (slices[0]->description.state == texture->description.state
				&& slices[0]->description.dimension == texture->description.dimension))) && allDone;
	}

	if (allDone) {
		Flag().fetch_or(TEXTUREARRAYRESOUCE_SLICE_MAPPED, std::memory_order_release);
	} else {
		Flag().fetch_or(RESOURCE_INVALID | TEXTUREARRAYRESOUCE_SLICE_MAPPED, std::memory_order_release);
	}

	return allDone;
}

bool TextureArrayResource::UnMap() {
	if (Flag().load(std::memory_order_acquire) & TEXTUREARRAYRESOUCE_SLICE_MAPPED) {
		for (size_t i = 0; i < slices.size(); i++) {
			slices[i]->UnMap();
		}
	}

	return BaseClass::UnMap();
}

