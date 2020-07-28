// Source.h
// By PaintDream (paintdream@paintdream.com)
// 2015-5-5
//

#ifndef __SOUNDCOMPONENT_H__
#define __SOUNDCOMPONENT_H__

#include "../../Component.h"
#include "../../../SnowyStream/SnowyStream.h"
#include "../../../SnowyStream/Resource/StreamResource.h"

namespace PaintsNow {
	namespace NsMythForest {
		class SoundComponent : public TAllocatedTiny<SoundComponent, Component> {
		public:
			enum {
				SOUNDCOMPONENT_ONLINE = COMPONENT_CUSTOM_BEGIN,
				SOUNDCOMPONENT_CUSTOM_BEGIN = COMPONENT_CUSTOM_BEGIN << 1
			};

			SoundComponent(TShared<NsSnowyStream::StreamResource> audioResource, IScript::Request::Ref callback);
			virtual ~SoundComponent();
			virtual TObject<IReflect>& operator () (IReflect& reflect) override;

			virtual void Initialize(Engine& engine, Entity* entity) override;
			virtual void Uninitialize(Engine& engine, Entity* entity) override;

			void Step(IScript::Request& request);
			int64_t GetDuration() const;
			void Seek(Engine& engine, double time);
			void Play(Engine& engine);
			void Pause(Engine& engine);
			void Stop(Engine& engine);
			void Rewind(Engine& engine);
			virtual void ScriptUninitialize(IScript::Request& request);
			bool IsOnline() const;
			bool IsPlaying() const;

		protected:
			IAudio::Decoder* audioStream;
			IAudio::Source* audioSource;
			IAudio::Buffer* audioBuffer;
			TShared<NsSnowyStream::StreamResource> audioResource;
			TWrapper<size_t> stepWrapper;
			IScript::Request::Ref callback;
		};
	}
}


#endif // __SOUNDCOMPONENT_H__