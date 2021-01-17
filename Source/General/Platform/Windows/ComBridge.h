// ComBridge.h
// But not cambridge :D
// PaintDream (paintdream@paintdream.com)
// 2015-7-4
//

#pragma once
#include "ComDef.h"
#include "../../../Core/Interface/IArchive.h"

namespace PaintsNow {
	class ComDispatch;
	class ComBridge : public TReflected<ComBridge, Bridge> {
	public:
		ComBridge(IThread& thread);
		virtual IReflectObject* Create(IScript::Request& request, IArchive& archive, const String& path, const String& data) override;
		virtual void Call(IReflectObject* object, const TProxy<>* p, IScript::Request& request) override;
		virtual std::unordered_map<Unique, ScriptReflect::Type>& GetReflectMap() override;
		virtual TObject<IReflect>& operator () (IReflect& reflect) override;
		std::unordered_map<size_t, Unique>& GetTypeMap();

	private:
		std::unordered_map<Unique, ScriptReflect::Type> reflectMap;
		std::unordered_map<size_t, Unique> typeMap;
	};
}

