#ifndef __ZAUDIOOPENAL_H__
#define __ZAUDIOOPENAL_H__

#include "../../../Interface/IAudio.h"
#include <cstdlib>
#define AL_LIBTYPE_STATIC
#include <AL/alc.h>
#include <AL/al.h>

namespace PaintsNow
{
	class ZAudioOpenAL final : public IAudio {
	public:
		ZAudioOpenAL();
		virtual ~ZAudioOpenAL();
		virtual IAudio::Buffer* CreateBuffer();
		virtual void SetBufferStream(Buffer* buffer, IAudio::Decoder& stream, bool online);
		virtual void SetBufferData(Buffer* buffer, const void* data, size_t length, Decoder::FORMAT dataType, size_t sampleRate);
		virtual void DeleteBuffer(Buffer* buffer);
		virtual IAudio::Source* CreateSource();
		virtual void DeleteSource(Source* sourceHandle);
		virtual void SetSourcePosition(Source* sourceHandle, const Float3& position);
		virtual void SetSourceVolume(Source* sourceHandle, float volume);
		virtual TWrapper<size_t> SetSourceBuffer(Source* sourceHandle, const Buffer* buffer);
		virtual void SetListenerPosition(const Float3& position);
		virtual void Play(Source* sourceHandle);
		virtual void Pause(Source* sourceHandle);
		virtual void Rewind(Source* sourceHandle);
		virtual void Stop(Source* sourceHandle);
		void ResetBuffer(Buffer* buffer);
		// virtual void Seek(Source* sourceHandle, IStreamBase::SEEK_OPTION option, long offset);

	private:
		void SwitchBufferType(IAudio::Buffer* buffer, bool online);
		ALCdevice* device;
		ALCcontext* context;
	};
}

#endif // __ZAUDIOOPENAL_H__
