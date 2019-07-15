
// IDatabase.h -- Database player interface
// By PaintDream (paintdream@paintdream.com)
// 2014-12-1
//

#ifndef __IDATABASE_H__
#define __IDATABASE_H__

#include "../../Core/PaintsNow.h"
#include "../../Core/Interface/IType.h"
#include "../../Core/Interface/IReflect.h"
#include "../../Core/Interface/IDevice.h"
#include "../../Core/Interface/IArchive.h"
#include <string>

namespace PaintsNow {
	class IDatabase : public IDevice {
	public:
		virtual ~IDatabase();
		class Database {};

		// class MetaData : public IIterator {};
		typedef IIterator MetaData;
		virtual Database* Connect(IArchive& archive, const String& target, const String& username, const String& password, bool createOnNonExist) = 0;
		virtual void Close(Database* database) = 0;
		virtual MetaData* Execute(Database* database, const String& statementTemplate, MetaData* postData) = 0;
	};
}

#endif // __IDATABASE_H__
