// StreamComponentModule.h
// By PaintDream (paintdream@paintdream.com)
// 2015-5-5
//

#ifndef __STREAMCOMPONENTMODULE_H__
#define __STREAMCOMPONENTMODULE_H__

#include "StreamComponent.h"
#include "../../Module.h"

namespace PaintsNow {
	namespace NsMythForest {
		class Entity;
		class StreamComponent;
		class StreamComponentModule : public TReflected<StreamComponentModule, ModuleImpl<StreamComponent> > {
		public:
			StreamComponentModule(Engine& engine);
			virtual ~StreamComponentModule();
			virtual TObject<IReflect>& operator () (IReflect& reflect) override;

			// static int main(int argc, char* argv[]);

		public:
			void RequestNew(IScript::Request& request, const UShort3& dimension, uint32_t cacheCount);
			void RequestSetStreamLoadHandler(IScript::Request& request, IScript::Delegate<StreamComponent> streamComponent, IScript::Request::Ref ref);
			void RequestSetStreamUnloadHandler(IScript::Request& request, IScript::Delegate<StreamComponent> streamComponent, IScript::Request::Ref ref);
		};
	}
}


#endif // __STREAMCOMPONENTMODULE_H__