#include "FontResource.h"
#include "../../../Core/Interface/IArchive.h"
#include "../../../General/Misc/ZMemoryStream.h"
#include "../../SnowyStream/SnowyStream.h"

using namespace PaintsNow;
using namespace PaintsNow::NsSnowyStream;

FontResource::FontResource(ResourceManager& manager, const String& uniqueID) : BaseClass(manager, uniqueID), font(nullptr), dim(512), weight(0) {}
FontResource::~FontResource() {}

bool FontResource::LoadExternalResource(Interfaces& interfaces, IStreamBase& streamBase, size_t length) {
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
		ReflectProperty(dim);
		ReflectProperty(weight);

		// runtime
		ReflectProperty(sliceMap)[Runtime];
		ReflectProperty(font)[Runtime];
	}

	return *this;
}

const FontResource::Char& FontResource::Get(IRender& render, IRender::Queue* queue, IFontBase& fontBase, IFontBase::FONTCHAR ch, int32_t size) {
	size = size == 0 ? 12 : size;

	SpinLock(critical);
	Slice& slice = sliceMap[size];
	SpinUnLock(critical);

	if (slice.fontSize == 0) { // not initialized?
		slice.fontSize = size;
		slice.font = font;
		slice.dim = dim;
	}

	const FontResource::Char& ret = slice.Get(render, queue, fontBase, ch);
	std::atomic<uint32_t>& lock = reinterpret_cast<std::atomic<uint32_t>&>(slice.critical);

	if (lock.load(std::memory_order_acquire) == 2u) {
		// need update
		Flag().fetch_or(TINY_MODIFIED, std::memory_order_acquire);
	}

	return ret;
}

FontResource::Slice::Slice(uint16_t fs, uint16_t d) : font(nullptr), fontSize(fs), critical(0), dim(d) {
	buffer.Resize(dim * dim * sizeof(uint8_t));
}

void FontResource::Slice::Uninitialize(IRender& render, IRender::Queue* queue, ResourceManager& resourceManager) {
	for (size_t i = 0; i < cacheTextures.size(); i++) {
		render.DeleteResource(queue, cacheTextures[i]());
	}

	cacheTextures.clear();
}

Short2Pair FontResource::Slice::AllocRect(IRender& render, IRender::Queue* queue, const Short2& size) {
	assert(size.x() <= dim);
	assert(size.y() <= dim);

	if (lastRect.second.y() + size.y() > dim || cacheTextures.empty()) {
		IRender::Resource* texture = render.CreateResource(queue, IRender::Resource::RESOURCE_TEXTURE);
		cacheTextures.push_back(texture);
		lastRect.first.y() = 0;
		lastRect.second.y() = size.y();
	}

	if (lastRect.second.x() + size.x() > dim) {
		// new line
		lastRect.second.x() = 0;
		lastRect.first.y() = lastRect.second.y();
	}

	uint16_t height = Math::Max((int16_t)(lastRect.second.y() - lastRect.first.y()), size.y());
	Short2Pair w;
	w.first.x() = lastRect.second.x();
	w.second.x() = w.first.x() + size.x();
	w.second.y() = lastRect.first.y() + height;
	w.first.y() = w.second.y() - size.y();
	lastRect = w;

	assert(!cacheTextures.empty());
	cacheTextures.back().Tag(1); // set dirty

	return w;
}

void FontResource::Slice::UpdateFontTexture(IRender& render, IRender::Queue* queue) {
	std::atomic<uint32_t>& lock = reinterpret_cast<std::atomic<uint32_t>&>(critical);

	if (SpinLock(lock) == 2u) {
		for (size_t i = 0; i < cacheTextures.size(); i++) {
			TTagged<IRender::Resource*, 2>& ptr = cacheTextures[i];

			if (ptr.Tag()) {
				IRender::Resource::TextureDescription desc;
				desc.data = buffer;
				desc.state.type = IRender::Resource::TextureDescription::TEXTURE_2D;
				desc.state.format = IRender::Resource::TextureDescription::UNSIGNED_BYTE;
				desc.state.layout = IRender::Resource::TextureDescription::R;
				desc.dimension.x() = dim;
				desc.dimension.y() = dim;

				render.UploadResource(queue, ptr(), &desc);
				ptr.Tag(0);
			}
		}
	}
	
	SpinUnLock(lock);
}

Short2 FontResource::Slice::GetTextureSize() const {
	return Short2(dim, dim);
}

uint16_t FontResource::GetFontTextureSize() const {
	return dim;
}

void FontResource::Update(IRender& render, IRender::Queue* queue) {
	if (Flag() & TINY_MODIFIED) {
		for (std::map<uint32_t, Slice>::iterator it = sliceMap.begin(); it != sliceMap.end(); it++) {
			std::atomic<uint32_t>& lock = reinterpret_cast<std::atomic<uint32_t>&>(it->second.critical);
			if (lock.load(std::memory_order_acquire) == 2u) {
				it->second.UpdateFontTexture(render, queue);
			}
		}

		Flag().fetch_and(~TINY_MODIFIED, std::memory_order_release);
	}
}

const FontResource::Char& FontResource::Slice::Get(IRender& render, IRender::Queue* queue, IFontBase& fontBase, IFontBase::FONTCHAR ch) {
	std::atomic<uint32_t>& lock = reinterpret_cast<std::atomic<uint32_t>&>(critical);
	SpinLock(lock);
	hmap::iterator p = cache.find(ch);
	if (p != cache.end()) {
		SpinUnLock(lock);
		return (*p).second;
	} else {
		assert(font != nullptr);
		Char c;
		String data;
		c.info = fontBase.RenderTexture(font, data, ch, fontSize, 0);
		c.rect = AllocRect(render, queue, Short2(c.info.width, c.info.height));
		c.textureResource = cacheTextures.back()();
		SpinUnLock(lock);

		Short2Pair& r = c.rect;
		uint8_t* target = buffer.GetData();

		const uint32_t* p = (const uint32_t*)data.data();
		for (int j = r.first.y(); j < r.second.y(); j++) {
			for (int i = r.first.x(); i < r.second.x(); i++) {
				target[j * dim + i] = *p++;
			}
		}

		SpinLock(lock);
		Char& ret = cache[ch] = c;
		SpinUnLock(lock, 2u);
	
		return ret;
	}
}

