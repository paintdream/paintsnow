// FlameWork.h
// By PaintDream (paintdream@paintdream.com)
// 2015-1-3
// 

#ifndef __FLAMEWORK_H__
#define __FLAMEWORK_H__

#include "../../Core/Interface/IScript.h"
#include "../BridgeSunset/BridgeSunset.h"
#include "Native.h"

namespace PaintsNow {
	namespace NsFlameWork {
		class FlameWork : public TReflected<FlameWork, IScript::Library> {
		public:
			FlameWork(IThread& threadApi, IScript& nativeScript, NsBridgeSunset::BridgeSunset& bridgeSunset);

			// function convension
			// int main(int argc, char* argv[]);
			// <Output> argv[0] -> returns a pointer to 

			TShared<Native> RequestCompileNativeCode(IScript::Request& request, const String& code);
			void RequestExecuteNative(IScript::Request& request, IScript::Delegate<Native> native, const String& entry, std::vector<String>& params);

			virtual TObject<IReflect>& operator () (IReflect& reflect) override;

		private:
			NsBridgeSunset::BridgeSunset& bridgeSunset;
			IScript& nativeScript;
		};
	}
}

#endif // __FLAMEWORK_H__