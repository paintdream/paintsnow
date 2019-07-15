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
		class FontResource : public TReflected<FontResource, GraphicResourceBase> {
		public:
			FontResource(ResourceManager& manager, const ResourceManager::UniqueLocation& uniqueID);
			virtual ~FontResource();
			virtual uint64_t GetMemoryUsage() const;
			virtual TObject<IReflect>& operator () (IReflect& reflect) override;
			virtual bool LoadExternalResource(IStreamBase& streamBase, size_t length);

			virtual void Upload(IRender& render, void* deviceContext) override;
			virtual void Download(IRender& render, void* deviceContext) override;
			virtual void Attach(IRender& render, void* deviceContext) override;
			virtual void Detach(IRender& render, void* deviceContext) override;

			struct Char {
				Char() { memset(this, 0, sizeof(*this)); }
				IFontBase::CHARINFO info;
				Int2Pair rect;
			};

			const Char& Get(IFontBase::FONTCHAR ch, int32_t size);
			IRender::Resource* GetFontTexture(uint32_t size, Int2& texSize);

		protected:
			class Slice {
			public:
				Slice(uint32_t size);
				const Char& Get(ResourceManager& resourceManager, IFontBase::FONTCHAR ch);
				void Initialize(ResourceManager& resourceManager);
				void Uninitialize(ResourceManager& resourceManager);
				IRender::Resource* GetFontTexture(ResourceManager& resourceManager);

				friend class FontResource;

			protected:
				typedef unordered_map<IFontBase::FONTCHAR, Char> hmap;
				hmap cache;
				Int2Pair lastRect;
				std::vector<uint32_t> buffer;
				int16_t width;
				uint16_t fontSize;
				IFontBase::Font* font;
				IRender::Resource* cacheTexture;
				bool modified;

				Int2 GetTextureSize() const;
				void UpdateFontTexture(ResourceManager& resourceManager);
				Int2Pair AllocRect(const Int2& size);
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
