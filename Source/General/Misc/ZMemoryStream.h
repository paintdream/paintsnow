// ZMemoryStream.h
// By PaintDream (paintdream@paintdream.com)
// 2015-1-19
//

#ifndef __ZMEMORYSTREAM_H__
#define __ZMEMORYSTREAM_H__

#include "../../Core/Interface/IType.h"
#include "../../Core/Interface/IStreamBase.h"

namespace PaintsNow {
	class ZMemoryStream : public IStreamBase {
	public:
		ZMemoryStream(size_t maxSize, bool autoSize = false, uint16_t alignment = 8);
		virtual ~ZMemoryStream();

		const void* GetBuffer() const;
		void* GetBuffer();

		void SetEnd();
		virtual bool Read(void* p, size_t& len);
		virtual bool Write(const void* p, size_t& len);
		virtual bool WriteDummy(size_t& len);
		virtual bool Seek(SEEK_OPTION option, long offset);
		virtual void Flush();
		size_t GetOffset() const;
		size_t GetTotalLength() const;
		size_t GetMaxLength() const;

		virtual bool Transfer(IStreamBase& stream, size_t& len);
		virtual IReflectObject* Clone() const;
		void Clear();
		bool Extend(size_t len);

	private:
		void Realloc(size_t newSize);
		bool CheckSize(size_t& len);

		uint8_t* buffer;
		size_t offset;
		size_t totalSize;
		size_t maxSize;
		uint16_t alignment;
		bool autoSize;
		uint8_t reserved;
	};
}

#endif // __ZMEMORYSTREAM_H__