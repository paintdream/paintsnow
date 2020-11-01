// ZArchiveVirtual -- 7z archiver
// By PaintDream (paintdream@paintdream.com)
//

#pragma once
#include "../../../../Core/Interface/IType.h"
#include "../../../../Core/Interface/IArchive.h"
#include "../../../../Core/Interface/IStreamBase.h"
#include <map>
#include <string>

namespace PaintsNow {
	class ZArchiveVirtual final : public IArchive {
	public:
		ZArchiveVirtual();
		~ZArchiveVirtual() override;

		const String& GetRootPath() const override;
		void SetRootPath(const String& path) override;
		IStreamBase* Open(const String& uri, bool write, size_t& length, uint64_t* lastModifiedTime = nullptr) override;
		void Query(const String& uri, const TWrapper<void, bool, const String&>& wrapper) const override;
		bool IsReadOnly() const override;
		bool Delete(const String& uri) override;
	};
}

