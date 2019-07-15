// LeavesApi.h
// By PaintDream (paintdream@paintdream.com)
// 2016-1-2
//

#ifndef __LEAVESAPI_H__
#define __LEAVESAPI_H__

#include "../../Core/Interface/IReflect.h"
#include "../../Core/Template/TFactory.h"
#include "../../General/Interface/IFrame.h"
#include "../../Core/Interface/IArchive.h"
#include "../../General/Interface/IAudio.h"
#include "../../General/Interface/INetwork.h"
#include "../../General/Interface/IRandom.h"
#include "../../General/Interface/IRender.h"
#include "../../Core/Interface/IArchive.h"
#include "../../General/Interface/IFontBase.h"
#include "../../Core/Interface/IScript.h"
#include "../../General/Interface/IImage.h"
#include "../../General/Interface/IDatabase.h"
#include "../../General/Interface/IAsset.h"
#include "../../General/Interface/ITimer.h"

#include <string>


namespace PaintsNow {
	namespace NsLeavesFlute {
		class LeavesFlute;
	}
	namespace NsLeavesFlute {
		class LeavesApi {
		public:
			virtual ~LeavesApi() {}
			enum RUNTIME_STATE {
				RUNTIME_INITIALIZE,
				// RUNTIME_RESET,
				RUNTIME_UNINITIALIZE
			};

			virtual void RegisterFactory(const String& factoryEntry, const String& name, const TWrapper<void*, const String&>* factoryBase) = 0;
			virtual void UnregisterFactory(const String& factoryEntry, const String& name) = 0;
			virtual void QueryFactory(const String& factoryEntry, const TWrapper<void, const String&, const TWrapper<void*, const String&>*>& callback) = 0;
			virtual void WriteString(String& target, const String& source) const = 0;
			virtual void RegisterRuntimeHook(const TWrapper<void, NsLeavesFlute::LeavesFlute*, RUNTIME_STATE>& proc) = 0;
		};
	}
}

#endif // __LEAVESAPI_H__