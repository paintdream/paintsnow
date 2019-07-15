// Repository.h
// PaintDream (paintdream@paintdream.com)
// 2019-11-1
//

#ifndef __REPOSITORY_H__
#define __REPOSITORY_H__

#include "IWidget.h"
#include "../../Core/Interface/IArchive.h"
#include "../../Core/System/Kernel.h"
#include "../../Core/Template/TAllocator.h"

namespace PaintsNow {
	namespace NsSnowyStream {
		class ResourceBase;
	}

	namespace NsLeavesWine {
		class Repository : public TReflected<Repository, WarpTiny>, public IWidget {
		public:
			Repository();
			virtual void TickRender(NsLeavesFlute::LeavesFlute& leavesFlute) override;

			struct Item final : public TAllocatedTiny<Item, SharedTiny> {
				enum { ITEM_DIRECTORY = SharedTiny::TINY_CUSTOM_BEGIN };
				Item() : parent(nullptr) {}
				Item* parent;
				String name;
				TShared<NsSnowyStream::ResourceBase> resource;
				std::vector<TShared<Item> > children;
			};

			void RenderItem(Item& item, uint32_t depth);
			void RefreshLocalFilesRecursive(IArchive& archive, const String& path, Item& item);
			void RefreshLocalFiles(IArchive& archive);

		protected:
			TShared<Item> rootItem;
			TShared<Item::Allocator> itemAllocator;
			String viewResourcePath;
			TShared<NsSnowyStream::ResourceBase> viewResource;
			TAtomic<uint32_t> critical;
		};
	}
}

#endif // __REPOSITORY_H__