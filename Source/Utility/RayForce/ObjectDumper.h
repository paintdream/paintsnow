// ObjectDumper.h
// By PaintDream (paintdream@paintdream.com)
// 2015-7-8
//

#ifndef __OBJECTDUMPER_H__
#define __OBJECTDUMPER_H__

#include "../../General/Misc/ZScriptReflect.h"

namespace PaintsNow {
	namespace NsRayForce {
		class Tunnel;
		class ObjectDumper : public ZScriptReflect {
		public:
			ObjectDumper(IScript::Request& request, Tunnel& tunnel, const std::map<Unique, Type>& m);
			virtual void Property(IReflectObject& s, Unique typeID, Unique refTypeID, const char* name, void* base, void* ptr, const MetaChainBase* meta);
			virtual void Method(Unique typeID, const char* name, const TProxy<>* p, const IReflect::Param& retValue, const std::vector<IReflect::Param>& params, const MetaChainBase* meta);

		private:
			IScript::Request& request;
			Tunnel& tunnel;
		};
	}
}
#endif // __OBJECTDUMPER_H__