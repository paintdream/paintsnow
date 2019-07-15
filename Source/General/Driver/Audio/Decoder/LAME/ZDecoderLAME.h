// ZDecoderLAME.h
// By PaintDream (paintdream@paintdream.com)
// 2014-12-13
//

#ifndef __ZDECODERLAME_H__
#define __ZDECODERLAME_H__

#ifndef __linux__
#include <lame.h>
#else
#include <lame/lame.h>
#endif

#include "../../../../Interface/IAudio.h"

namespace PaintsNow {
	class ZDecoderLAME final : public IAudio::Decoder {
	public:
		ZDecoderLAME();
		// derived from IStreamBase
		virtual void Flush();
		virtual long GetRemaining() const;
		virtual bool Read(void* p, size_t& len);
		virtual bool Write(const void* p, size_t& len);
		virtual bool Transfer(IStreamBase& stream, size_t& len);
		virtual bool WriteDummy(size_t& len);
		virtual bool Seek(IStreamBase::SEEK_OPTION option, long offset);

		// derived from IFilterBase
		virtual void Attach(IStreamBase& inputStream);
		virtual void Detach();

		// derived from IAudio::Decoder
		// Notice that these functions are valid only after a successful call to Read().
		virtual size_t GetSampleRate() const;
		virtual FORMAT GetFormat() const;
		virtual size_t GetSampleCount() const;
		enum { SAMPLE_COUNT = 4096 };
		enum { BUFFER_LENGTH = 4096 };

	private:
		IStreamBase* sourceStream;
		size_t sampleRate;
		size_t sampleCount;
		size_t bitrate;
		FORMAT format;
		hip_t hip;
		short extra[2][SAMPLE_COUNT];
		int extraStart;
		int extraCount;
	};
}

#endif // __ZDECODERLAME_H__
