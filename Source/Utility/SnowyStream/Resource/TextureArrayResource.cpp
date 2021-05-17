#include "TextureArrayResource.h"
#include "../Manager/RenderResourceManager.h"
using namespace PaintsNow;

TextureArrayResource::TextureArrayResource(ResourceManager& manager, const String& uniqueID) : BaseClass(manager, uniqueID) {}

void TextureArrayResource::Refresh(IRender& device, void* deviceContext) {}
void TextureArrayResource::Download(IRender& device, void* deviceContext) {}
void TextureArrayResource::Upload(IRender& device, void* deviceContext) {}
void TextureArrayResource::Attach(IRender& device, void* deviceContext) {}
void TextureArrayResource::Detach(IRender& device, void* deviceContext) {}
