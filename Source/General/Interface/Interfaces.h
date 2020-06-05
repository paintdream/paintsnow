// Interfaces.h
// By PaintDream (paintdream@paintdream.com)
// 2018-4-4
//

#ifndef __INTERFACES_H__
#define __INTERFACES_H__

#ifndef _CRT_SECURE_NO_WARNINGS
#define _CRT_SECURE_NO_WARNINGS
#endif

#include "IRender.h"
#include "IAudio.h"
#include "IDatabase.h"
#include "IDebugger.h"
#include "IFontBase.h"
#include "IFrame.h"
#include "IImage.h"
#include "INetwork.h"
#include "IRandom.h"
#include "../../Core/Interface/IArchive.h"
#include "../../Core/Interface/IFilterBase.h"
#include "../../Core/Interface/IScript.h"
#include "../../Core/Interface/IThread.h"
#include "ITimer.h"
#include "ITunnel.h"

namespace PaintsNow {

	class Interfaces {
	public:
		Interfaces(IArchive& parchive, IAudio& paudio, IDatabase& pdatabase, 
			IFilterBase& passetFilterBase, IFilterBase& paudioFilterBase, IFontBase& pfontBase, IFrame& pframe, IImage& image, INetwork& pnetwork, IRandom& prandom, IRender& prender,
			IScript& pscript, IScript& pnativeScript, IThread& pthread, ITimer& ptimer, ITunnel& ptunnel);

		IArchive& archive;
		IAudio& audio;
		IDatabase& database;
		IFilterBase& assetFilterBase;
		IFilterBase& audioFilterBase;
		IFontBase& fontBase;
		IFrame& frame;
		IImage& image;
		INetwork& network;
		IRandom& random;
		IRender& render;
		IScript& script;
		IScript& nativeScript;
		IThread& thread;
		ITimer& timer;
		ITunnel& tunnel;
	};
}


#endif // __INTERFACES_H__