// LeavesApi.h
// By PaintDream (paintdream@paintdream.com)
// 2016-1-2
//

#pragma once
#include "../../Core/Interface/IReflect.h"
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
	class LeavesApi {
	public:
		virtual ~LeavesApi() {}
		virtual void RegisterFactory(const String& factoryEntry, const String& name, const TWrapper<IDevice*>& factoryBase) = 0;
		virtual void UnregisterFactory(const String& factoryEntry, const String& name) = 0;
	};
}

