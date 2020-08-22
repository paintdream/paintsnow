// Remembery.h
// By PaintDream (paintdream@paintdream.com)
// 2015-6-15
//

#pragma once
#include "Hive.h"
#include "../BridgeSunset/BridgeSunset.h"

namespace PaintsNow {
	class Remembery : public TReflected<Remembery, IScript::Library> {
	public:
		Remembery(IThread& threadApi, IArchive& archive, IDatabase& databaseFactory, BridgeSunset& bridgeSunset);
		virtual ~Remembery();
		virtual TObject<IReflect>& operator () (IReflect& reflect) override;

		TShared<Hive> RequestNewDatabase(IScript::Request& request, const String& path, const String& username, const String& password, bool createOnNonExist);
		TShared<Honey> RequestExecute(IScript::Request& request, IScript::Delegate<Hive> hive, const String& sql, HoneyData& honeyData);
		void RequestStep(IScript::Request& request, IScript::Delegate<Honey> honey, uint32_t count);

	private:
		BridgeSunset& bridgeSunset;
		IDatabase& databaseFactory;
		IArchive& archive;
	};
}

