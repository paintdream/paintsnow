// DirectScriptBridge.h
// By PaintDream (paintdream@paintdream.com)
// 2015-10-18
//

#ifndef __DIRECTSCRIPTBRIDGE_H__
#define __DIRECTSCRIPTBRIDGE_H__

#include "../RayForce.h"

namespace PaintsNow {
	namespace NsRayForce {
		class DirectScriptBridge : public Bridge {
		public:
			DirectScriptBridge(IThread& thread, const TFactoryBase<IScript>& factory);
			virtual ~DirectScriptBridge();
			virtual IReflectObject* Create(IScript::Request& request, IArchive& archive, const String& path, const String& data);
			virtual void Call(IReflectObject* object, const TProxy<>* p, IScript::Request& request);
			virtual std::map<Unique, ZScriptReflect::Type>& GetReflectMap();

		private:
			std::map<Unique, ZScriptReflect::Type> reflectMap;
			const TFactoryBase<IScript>& factoryBase;
		};
	}
}

#endif // __DIRECTSCRIPTBRIDGE_H__