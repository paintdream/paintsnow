// ZDatabaseSqlite.h
// By PaintDream (paintdream@paintdream.com)
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
		virtual ~ZDatabaseSqlite();
		virtual Database* Connect(IArchive& archive, const String& target, const String& username, const String& password, bool createOnNonExist);
		virtual void Close(Database* database);
		virtual MetaData* Execute(Database* database, const String& statementTemplate, MetaData* postData);
	};
}

