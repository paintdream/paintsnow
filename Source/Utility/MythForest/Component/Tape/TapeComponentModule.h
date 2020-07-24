// TapeComponent.h
// By PaintDream (paintdream@paintdream.com)
// 2018-1-4
//

#ifndef __TAPECOMPONENTMODULE_H__
#define __TAPECOMPOENNTMODULE_H__

#include "TapeComponent.h"
#include "../../Module.h"

namespace PaintsNow {
	namespace NsMythForest {
		class TapeComponent;
		class TapeComponentModule  : public TReflected<TapeComponentModule, ModuleImpl<TapeComponent> > {
		public:
			TapeComponentModule(Engine& engine);
			virtual TObject<IReflect>& operator () (IReflect& reflect) override;

			TShared<TapeComponent> RequestNew(IScript::Request& request, IScript::Delegate<SharedTiny> streamHolder);
			std::pair<int64_t, String> RequestRead(IScript::Request& request, IScript::Delegate<TapeComponent> tapeComponent);
			bool RequestWrite(IScript::Request& request, IScript::Delegate<TapeComponent> tapeComponent, int64_t seq, const String& data);
			bool RequestSeek(IScript::Request& request, IScript::Delegate<TapeComponent> tapeComponent, int64_t seq);
		};
	}
}


#endif // __TAPECOMPONENTMODULE_H__