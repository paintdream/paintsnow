// FontResource.h
// By PaintDream (paintdream@paintdream.com)
// 2018-3-11
//

#ifndef __FONTRESOURCE_H__
#define __FONTRESOURCE_H__

#include "GraphicResourceBase.h"
#include "../../../General/Interface/IFontBase.h"

namespace PaintsNow {
	namespace NsSnowyStream {
		class FontResource : public TReflected<FontResource, DeviceResourceBase<IFontBase> > {
		public:
			FontResource(ResourceManager& manager, const ResourceManager::UniqueLocation& uniqueID);
			virtual ~FontResource();
			virtual TObject<IReflect>& operator () (IReflect& reflect) override;
			virtual bool LoadExternalResource(IStreamBase& streamBase, size_t length);

			virtual void Upload(IFontBase& font, void* deviceContext) override;
			virtual void Download(IFontBase& font, void* deviceContext) override;
			virtual void Attach(IFontBase& font, void* deviceContext) override;
			virtual void Detach(IFontBase& font, void* deviceContext) override;

			struct Char {
				Char() { memset(this, 0, sizeof(*this)); }
				IFontBase::CHARINFO info;
				Short2Pair rect;
			};

			const Char& Get(IRender& render, IRender::Queue* queue, IFontBase& fontBase, IFontBase::FONTCHAR ch, int32_t size);
			IRender::Resource* GetFontTexture(IRender& render, IRender::Queue* queue, uint32_t size, Short2& texSize);

		protected:
			class Slice {
			public:
				Slice(uint16_t size = 0, uint16_t dim = 0);
				const Char& Get(ResourceManager& resourceManager, IFontBase& font, IFontBase::FONTCHAR ch);
				void Initialize(IRender& render, IRender::Queue* queue, ResourceManager& resourceManager);
				void Uninitialize(IRender& render, IRender::Queue* queue, ResourceManager& resourceManager);
				IRender::Resource* GetFontTexture(IRender& render, IRender::Queue* queue, ResourceManager& resourceManager);

				friend class FontResource;

			protected:
				typedef unordered_map<IFontBase::FONTCHAR, Char> hmap;
				hmap cache;
				Short2Pair lastRect;
				Bytes buffer;
				uint16_t dim;
				uint16_t fontSize;
				IFontBase::Font* font;
				IRender::Resource* cacheTexture;
				bool modified;
				bool reserved[3];

				Short2 GetTextureSize() const;
				void UpdateFontTexture(IRender& render, IRender::Queue* queue, ResourceManager& resourceManager);
				Short2Pair AllocRect(const Short2& size);
			};

		private:
			std::map<uint32_t, Slice> sliceMap;
			IFontBase::Font* font;
			String rawFontData;
			float reinforce;
		};
	}
}


#endif // __FONTRESOURCE_H__
