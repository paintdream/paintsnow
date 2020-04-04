// IDebugger.h
// By PaintDream (paintdream@paintdream.com)
// 2015-6-16
//

#ifndef __IDEBUGGER_H__
#define __IDEBUGGER_H__

#include "../../Core/Interface/IType.h"
#include "../../Core/Template/TProxy.h"
#include "../../Core/Interface/IDevice.h"
#include <string>

namespace PaintsNow {
	class IDebugger : public IDevice {
	public:
		virtual ~IDebugger();
		virtual void SetDumpHandler(const String& path, const TWrapper<bool>& handler) = 0;
		virtual void StartDump(const String& options) = 0;
		virtual void EndDump() = 0;
		virtual void InvokeDump(const String& options) = 0;
	};
}


#endif // __IDEBUGEER_H__