// Bridge.h
// By PaintDream (paintdream@paintdream.com)
// 2015-7-8
//

#ifndef __BRIDGE_H__
#define __BRIDGE_H__

#include "../../Core/Interface/IReflect.h"
#include "../../Core/Interface/IArchive.h"
#include "../../Core/Interface/IScript.h"
#include "../../General/Misc/ZScriptReflect.h"

#include "Tunnel.h"

namespace PaintsNow {
	namespace NsRayForce {
		class BridgeN {};
		class Bridge : public TReflected<Bridge, IReflectObjectComplex>, public ISyncObject {
		public:
			Bridge(IThread& thread);
			virtual ~Bridge();

			virtual IReflectObject* Create(IScript::Request& request, IArchive& archive, const String& path, const String& data) = 0;
			virtual void Call(IReflectObject* object, const TProxy<>* p, IScript::Request& request) = 0;
			virtual std::map<Unique, ZScriptReflect::Type>& GetReflectMap() = 0;
			// virtual void Dump(IScript::Request& request, Tunnel& tunnel, IReflectObject* object) = 0;
		};
	}
}


#endif // __BRIDGE_H__