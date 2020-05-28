#include "FontResource.h"
#include "../../../Core/Interface/IArchive.h"
#include "../../../General/Misc/ZMemoryStream.h"
#include "../../SnowyStream/SnowyStream.h"

using namespace PaintsNow;
using namespace PaintsNow::NsSnowyStream;

FontResource::FontResource(ResourceManager& manager, const ResourceManager::UniqueLocation& uniqueID) : BaseClass(manager, uniqueID), font(nullptr), reinforce(0) {}
FontResource::~FontResource() {}

bool FontResource::LoadExternalResource(IStreamBase& streamBase, size_t length) {
	rawFontData.resize(length);
	return streamBase.ReadBlock(const_cast<char*>(rawFontData.data()), length);
}

void FontResource::Attach(IFontBase& fontBase, void* deviceContext) {
}

void FontResource::Detach(IFontBase& fontBase, void* deviceContext) {
	SnowyStream* snowyStream = reinterpret_cast<SnowyStream*>(deviceContext);
	IRender& render = snowyStream->GetInterfaces().render;
	IRender::Queue* queue = snowyStream->GetResourceQueue();

	for (std::map<uint32_t, Slice>::iterator it = sliceMap.begin(); it != sliceMap.end(); ++it) {
		it->second.Uninitialize(render, queue, resourceManager);
	}

	sliceMap.clear();

	if (font != nullptr) {
		fontBase.Close(font);
		font = nullptr;
	}
}

void FontResource::Upload(IFontBase& fontBase, void* deviceContext) {
	if (font == nullptr && !rawFontData.empty()) {
		// load font resource from memory
		ZMemoryStream ms(rawFontData.size(), false);
		size_t len = rawFontData.size();
		if (ms.WriteBlock(rawFontData.data(), len)) {
			ms.Seek(IStreamBase::BEGIN, 0);
			font = fontBase.Load(ms, len);
		}

		if (font == nullptr) {
			// report error
			resourceManager.Report(String("Unable to load font resource: ") + uniqueLocation);
		}
	}
}

void FontResource::Download(IFontBase& fontBase, void* deviceContext) {
}

TObject<IReflect>& FontResource::operator () (IReflect& reflect) {
	BaseClass::operator () (reflect);

	if (reflect.IsReflectProperty()) {
		// serializable
		ReflectProperty(rawFontData);
		ReflectProperty(reinforce);

		// runtime
		ReflectProperty(sliceMap)[Runtime];
		ReflectProperty(font)[Runtime];
	}

	return *this;
}

IRender::Resource* FontResource::GetFontTexture(IRender& render, IRender::Queue* queue, uint32_t size, Short2& texSize) {
	std::map<uint32_t, Slice>::iterator it = sliceMap.find(size);
	if (it == sliceMap.end()) {
		return nullptr;
	} else {
		texSize = it->second.GetTextureSize();
		return it->second.GetFontTexture(render, queue, resourceManager);
	}
}

const FontResource::Char& FontResource::Get(IRender& render, IRender::Queue* queue, IFontBase& fontBase, IFontBase::FONTCHAR ch, int32_t size) {
	size = size == 0 ? 12 : size;

	Slice& slice = sliceMap[size];
	if (slice.fontSize == 0) { // not initialized?
		slice.fontSize = size;
		slice.font = font;
		slice.Initialize(render, queue, resourceManager);
	}

	return slice.Get(resourceManager, fontBase, ch);
}

FontResource::Slice::Slice(uint16_t fs, uint16_t d) : cacheTexture(nullptr), font(nullptr), fontSize(fs), modified(false), dim(d) {
	buffer.Resize(dim * dim * sizeof(uint8_t));
}

void FontResource::Slice::Initialize(IRender& render, IRender::Queue* queue, ResourceManager& resourceManager) {
	assert(cacheTexture == nullptr);
	assert(false);
	
	cacheTexture = render.CreateResource(queue, IRender::Resource::RESOURCE_TEXTURE);
}

void FontResource::Slice::Uninitialize(IRender& render, IRender::Queue* queue, ResourceManager& resourceManager) {
	if (cacheTexture != nullptr) {
		render.DeleteResource(queue, cacheTexture);
		cacheTexture = nullptr;
	}
}

Short2Pair FontResource::Slice::AllocRect(const Short2& size) {
	assert(size.x() <= (int)dim);
	if (lastRect.second.x() + size.x() > (int)dim) {
		// new line
		lastRect.second.x() = 0;
		lastRect.first.y() = lastRect.second.y();
	}

	uint16_t height = Max((int16_t)(lastRect.second.y() - lastRect.first.y()), size.y());
	Short2Pair w;
	w.first.x() = lastRect.second.x();
	w.second.x() = w.first.x() + size.x();
	w.second.y() = lastRect.first.y() + height;
	w.first.y() = w.second.y() - size.y();
	lastRect = w;

	return w;
}

IRender::Resource* FontResource::Slice::GetFontTexture(IRender& render, IRender::Queue* queue, ResourceManager& resourceManager) {
	if (modified) {
		UpdateFontTexture(render, queue, resourceManager);
		modified = false;
	}

	return cacheTexture;
}

void FontResource::Slice::UpdateFontTexture(IRender& render, IRender::Queue* queue, ResourceManager& resourceManager) {
	if (lastRect.second.y() != 0) {
		IRender::Resource::TextureDescription desc;
		desc.data = buffer;
		desc.state.type = IRender::Resource::TextureDescription::TEXTURE_2D;
		desc.state.format = IRender::Resource::TextureDescription::UNSIGNED_BYTE;
		desc.state.layout = IRender::Resource::TextureDescription::R;
		desc.dimension.x() = dim;
		desc.dimension.y() = dim;

		render.UploadResource(queue, cacheTexture, &desc);
	}
}

Short2 FontResource::Slice::GetTextureSize() const {
	return Short2(dim, dim);
}

const FontResource::Char& FontResource::Slice::Get(ResourceManager& resourceManager, IFontBase& fontBase, IFontBase::FONTCHAR ch) {
	hmap::iterator p = cache.find(ch);
	if (p != cache.end()) {
		return (*p).second;
	} else {
		assert(font != nullptr);
		Char c;
		String data;
		c.info = fontBase.RenderTexture(font, data, ch, fontSize, 0);
		c.rect = AllocRect(Short2(c.info.width, c.info.height));

		Short2Pair& r = c.rect;
		uint8_t* target = buffer.GetData();

		const uint32_t* p = (const uint32_t*)data.data();
		for (int j = r.first.y(); j < r.second.y(); j++) {
			for (int i = r.first.x(); i < r.second.x(); i++) {
				target[j * dim + i] = *p++;
			}
		}

		modified = true;
		return cache[ch] = c;
	}
}

