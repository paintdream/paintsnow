// StreamComponentModule.h
// By PaintDream (paintdream@paintdream.com)
// 2015-5-5
//

#pragma once
#include "StreamComponent.h"
#include "../../Module.h"

namespace PaintsNow {
	class Entity;
	class StreamComponent;
	class StreamComponentModule : public TReflected<StreamComponentModule, ModuleImpl<StreamComponent> > {
	public:
		StreamComponentModule(Engine& engine);
		virtual ~StreamComponentModule();
		virtual TObject<IReflect>& operator () (IReflect& reflect) override;

		// static int main(int argc, char* argv[]);

	public:
		TShared<StreamComponent> RequestNew(IScript::Request& request, const UShort3& dimension, uint16_t cacheCount);
		void RequestSetStreamLoadHandler(IScript::Request& request, IScript::Delegate<StreamComponent> streamComponent, IScript::Request::Ref ref);
		void RequestSetStreamUnloadHandler(IScript::Request& request, IScript::Delegate<StreamComponent> streamComponent, IScript::Request::Ref ref);
	};
}

