// Native.h
// 2016-12-3
// By PaintDream (paintdream@paintdream.com)
//

#ifndef __NATIVE_H__
#define __NATIVE_H__

#include "../../Core/Interface/IScript.h"
#include "../../Core/System/Kernel.h"

namespace PaintsNow {
	namespace NsFlameWork {
		class Native : public TReflected<Native, WarpTiny> {
		public:
			Native(IScript& nativeScript);
			virtual ~Native();
			bool Compile(const String& code);
			bool Execute(IScript::Request& request, const String& name, const std::vector<String>& parameters);

		private:
			IScript::Request* nativeRequest;
			typedef int (*Routine)(int argc, char* argv[]);
			std::map<String, IScript::Request::Ref> routineRefs;
			bool compiled;
		};
	}
}


#endif // __NATIVE_H__