// ZLocalStream.h
// By PaintDream (paintdream@paintdream.com)
// 2015-1-19
//

#ifndef __ZLOCALSTREAM_H__
#define __ZLOCALSTREAM_H__

#include "../../Core/Interface/IType.h"
#include "../../Core/Interface/IStreamBase.h"
#include "../../Core/Template/TBuffer.h"

namespace PaintsNow {
	class ZLocalStream : public IStreamBase {
	public:
		ZLocalStream();
		virtual ~ZLocalStream();

		virtual bool operator << (IStreamBase& stream) override;
		virtual bool operator >> (IStreamBase& stream) const override;

		virtual bool Read(void* p, size_t& len) override;
		virtual bool Write(const void* p, size_t& len) override;
		virtual bool WriteDummy(size_t& len) override;
		virtual bool Seek(SEEK_OPTION option, long offset) override;
		virtual void Flush() override;
		virtual bool Transfer(IStreamBase& stream, size_t& len) override;

		Bytes& GetPayload();
		const Bytes& GetPayload() const;
	
	protected:
		void Cleanup();

		IStreamBase* baseStream;
		uint32_t length;
		Bytes payload;
	};
}

#endif // __ZLOCALSTREAM_H__