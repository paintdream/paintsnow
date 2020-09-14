// ProxyStub.h
// By PaintDream
// 2016-3-22
//

#pragma once
#include "../../Core/Interface/IScript.h"
#include "../../Core/Interface/IThread.h"
#include "../../General/Interface/ITunnel.h"
#include "../../Core/System/Kernel.h"
#include "../../General/Misc/RemoteProxy.h"

namespace PaintsNow {
	class ProxyStub : public TReflected<ProxyStub, WarpTiny> {
	public:
		ProxyStub(IThread& thread, ITunnel& tunnel, const String& entry, const TWrapper<void, IScript::Request&, bool, RemoteProxy::STATUS, const String&>& statusHandler = TWrapper<void, IScript::Request&, bool, RemoteProxy::STATUS, const String&>());
		void ScriptInitialize(IScript::Request& request) override;
		void ScriptUninitialize(IScript::Request& request) override;

	protected:
		IScript::Object* StaticCreate(const String& entry);
		RemoteProxy remoteProxy;
	};
}

