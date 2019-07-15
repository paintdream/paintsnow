// Hive.h
// By PaintDream (paintdream@paintdream.com)
// 2015-12-31
//

#ifndef __HIVE_H__
#define __HIVE_H__

#include "../../Core/System/Kernel.h"
#include "Honey.h"

namespace PaintsNow {
	namespace NsRemembery {
		class Hive : public TReflected<Hive, WarpTiny> {
		public:
			Hive(IDatabase& base, IDatabase::Database* database);
			virtual ~Hive();

			virtual TObject<IReflect>& operator () (IReflect& reflect) override;
			TShared<Honey> Execute(const String& sql, HoneyData& honeyData);

		private:
			IDatabase& base;
			IDatabase::Database* database;
		};
	}
}


#endif // __HIVE_H__