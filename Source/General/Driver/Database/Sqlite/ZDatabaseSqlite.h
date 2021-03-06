// ZDatabaseSqlite.h
// PaintDream (paintdream@paintdream.com)
// 2015-12-30
//

#pragma once
#include "../../../Interface/IDatabase.h"
extern "C" {
	#include "Core/sqlite3.h"
}

namespace PaintsNow {
	class ZDatabaseSqlite final : public IDatabase {
	public:
		ZDatabaseSqlite();
		~ZDatabaseSqlite() override;
		Database* Connect(IArchive& archive, const String& target, const String& username, const String& password, bool createOnNonExist) override;
		void Close(Database* database) override;
		MetaData* Execute(Database* database, const String& statementTemplate, MetaData* postData) override;
	};
}

