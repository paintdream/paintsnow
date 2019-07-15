// Remembery.h
// By PaintDream (paintdream@paintdream.com)
// 2015-6-15
//

#ifndef __REMEMBERY_H__
#define __REMEMBERY_H__

#include "Hive.h"
#include "../BridgeSunset/BridgeSunset.h"

namespace PaintsNow {
	namespace NsRemembery {
		class Remembery : public TReflected<Remembery, IScript::Library> {
		public:
			Remembery(IThread& threadApi, IArchive& archive, IDatabase& databaseFactory, NsBridgeSunset::BridgeSunset& bridgeSunset);
			virtual ~Remembery();
			virtual TObject<IReflect>& operator () (IReflect& reflect) override;

			void RequestNewDatabase(IScript::Request& request, const String& path, const String& username, const String& password, bool createOnNonExist);
			void RequestExecute(IScript::Request& request, IScript::Delegate<Hive> hive, const String& sql, HoneyData& honeyData);
			void RequestStep(IScript::Request& request, IScript::Delegate<Honey> honey, uint32_t count);

		private:
			NsBridgeSunset::BridgeSunset& bridgeSunset;
			IDatabase& databaseFactory;
			IArchive& archive;
		};
	}
}

#endif // __REMEMBERY_H__