// SoundComponentModule.h
// By PaintDream (paintdream@paintdream.com)
// 2015-5-5
//

#pragma once
#include "SoundComponent.h"
#include "../../Module.h"

namespace PaintsNow {
	class Entity;
	class SoundComponent;
	class SoundComponentModule : public TReflected<SoundComponentModule, ModuleImpl<SoundComponent> > {
	public:
		SoundComponentModule(Engine& engine);
		virtual ~SoundComponentModule();
		virtual TObject<IReflect>& operator () (IReflect& reflect) override;

		// static int main(int argc, char* argv[]);

	public:
		TShared<SoundComponent> RequestNew(IScript::Request& request, String path, IScript::Request::Ref callback);
		int64_t RequestGetSourceDuration(IScript::Request& request, IScript::Delegate<SoundComponent> source);
		void RequestMoveSource(IScript::Request& request, IScript::Delegate<SoundComponent> source, Float3& pos);
		void RequestSeekSource(IScript::Request& request, IScript::Delegate<SoundComponent> source, double time);
		void RequestPlaySource(IScript::Request& request, IScript::Delegate<SoundComponent> source);
		void RequestPauseSource(IScript::Request& request, IScript::Delegate<SoundComponent> source);
		void RequestStopSource(IScript::Request& request, IScript::Delegate<SoundComponent> source);
		void RequestRewindSource(IScript::Request& request, IScript::Delegate<SoundComponent> source);
		bool RequestIsSourcePaused(IScript::Request& request, IScript::Delegate<SoundComponent> source);
	};
}

