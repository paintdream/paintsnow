// ComBridge.h
// But not cambridge :D
// By PaintDream (paintdream@paintdream.com)
// 2015-7-4
//

#ifndef __COMBRIDGE_H__
#define __COMBRIDGE_H__

#include "../RayForce.h"
#include "../../../General/Misc/ZScriptReflect.h"

#if defined(_WIN32)
namespace PaintsNow {
	namespace NsRayForce {
		class ComDispatch;
		class ComBridge : public TReflected<ComBridge, Bridge> {
		public:
			ComBridge(IThread& thread);
			virtual IReflectObject* Create(IScript::Request& request, IArchive& archive, const String& path, const String& data);
			virtual void Call(IReflectObject* object, const TProxy<>* p, IScript::Request& request);
			virtual std::map<Unique, ZScriptReflect::Type>& GetReflectMap();
			virtual std::map<size_t, Unique>& GetTypeMap();
			virtual TObject<IReflect>& operator () (IReflect& reflect) override;

		private:
			std::map<Unique, ZScriptReflect::Type> reflectMap;
			std::map<size_t, Unique> typeMap;
		};
	}
}

#endif // _WIN32

#endif // __COMBRIDGE_H__