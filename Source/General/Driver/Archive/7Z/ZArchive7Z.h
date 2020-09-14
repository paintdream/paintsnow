// ZArchive7Z -- 7z archiver
// By PaintDream (paintdream@paintdream.com)
//

#pragma once
#include "../../../../Core/Interface/IType.h"
#include "../../../../Core/Interface/IArchive.h"
#include "../../../../Core/Interface/IStreamBase.h"
#include <map>
#include <string>

extern "C" {
	#include "../../Filter/LZMA/Core/7z.h"
	#include "../../Filter/LZMA/Core/7zFile.h"
	#include "../../Filter/LZMA/Core/7zCrc.h"
	#include "../../Filter/LZMA/Core/7zAlloc.h"
	#include "../../Filter/LZMA/Core/7zBuf.h"
}

namespace PaintsNow {
	class ZArchive7Z final : public IArchive {
	public:
		ZArchive7Z(IStreamBase& stream, size_t len);
		~ZArchive7Z() override;
		const String& GetRootPath() const override;
		void SetRootPath(const String& path) override;
		IStreamBase* Open(const String& uri, bool write, size_t& length, uint64_t* lastModifiedTime = nullptr) override;
		void Query(const String& uri, const TWrapper<void, bool, const String&>& wrapper) const override;
		bool IsReadOnly() const override;
		bool Delete(const String& uri) override;

		static int main(int argc, char* argv[]);

		IStreamBase& GetStream();
		int64_t GetPos() const;
		void SetPos(int64_t s);
		int64_t GetLength() const;

	private:
		bool Open();

	private:
		IStreamBase& stream;
		CFileInStream archiveStream;
		CLookToRead lookStream;
		CSzArEx db;
		ISzAlloc allocImp;
		ISzAlloc allocTempImp;
		int64_t pos;
		int64_t size;
		bool opened;
		std::map<String, std::pair<UInt32, bool> > mapPathToID;
	};
}

