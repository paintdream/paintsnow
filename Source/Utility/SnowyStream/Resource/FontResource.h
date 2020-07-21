// FontResource.h
// By PaintDream (paintdream@paintdream.com)
// 2018-3-11
//

#ifndef __FONTRESOURCE_H__
#define __FONTRESOURCE_H__

#include "GraphicResourceBase.h"
#include "TextureResource.h"
#include "../ResourceManager.h"
#include "../../../General/Interface/IFontBase.h"
#include "../../../Core/Template/TTagged.h"
#include "../../../Core/Template/TMap.h"

namespace PaintsNow {
	namespace NsSnowyStream {
		class FontResource : public TReflected<FontResource, DeviceResourceBase<IFontBase> >, public IDataUpdater {
		public:
			FontResource(ResourceManager& manager, const String& uniqueID);
			virtual ~FontResource();
			virtual TObject<IReflect>& operator () (IReflect& reflect) override;
			virtual bool LoadExternalResource(Interfaces& interfaces, IStreamBase& streamBase, size_t length) override;

			virtual void Upload(IFontBase& font, void* deviceContext) override;
			virtual void Download(IFontBase& font, void* deviceContext) override;
			virtual void Attach(IFontBase& font, void* deviceContext) override;
			virtual void Detach(IFontBase& font, void* deviceContext) override;
			virtual void Update(IRender& render, IRender::Queue* queue) override;
			uint16_t GetFontTextureSize() const;

			struct Char {
				Char() { memset(this, 0, sizeof(*this)); }

				IFontBase::CHARINFO info;
				Short2Pair rect;
				IRender::Resource* textureResource;
			};

			const Char& Get(IRender& render, IRender::Queue* queue, IFontBase& fontBase, IFontBase::FONTCHAR ch, int32_t size);

		protected:
			class Slice {
			public:
				Slice(uint16_t size = 0, uint16_t dim = 0);
				const Char& Get(IRender& render, IRender::Queue* queue, IFontBase& font, IFontBase::FONTCHAR ch);
				void Uninitialize(IRender& render, IRender::Queue* queue, ResourceManager& resourceManager);

				friend class FontResource;

			protected:
				typedef std::unordered_map<IFontBase::FONTCHAR, Char> hmap;
				uint32_t critical;
				hmap cache;
				Short2Pair lastRect;
				Bytes buffer;
				uint16_t dim;
				uint16_t fontSize;
				IFontBase::Font* font;
				std::vector<TTagged<IRender::Resource*, 2> > cacheTextures;

				Short2 GetTextureSize() const;
				void UpdateFontTexture(IRender& render, IRender::Queue* queue);
				Short2Pair AllocRect(IRender& render, IRender::Queue* queue, const Short2& size);
			};

		private:
			std::map<uint32_t, Slice> sliceMap;
			IFontBase::Font* font;
			String rawFontData;
			uint16_t dim;
			uint16_t weight;
		};
	}
}


#endif // __FONTRESOURCE_H__
