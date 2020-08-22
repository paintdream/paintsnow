#include "Remembery.h"

using namespace PaintsNow;

Remembery::Remembery(IThread& threadApi, IArchive& ar, IDatabase& db, BridgeSunset& bs) : archive(ar), databaseFactory(db), bridgeSunset(bs) {}

Remembery::~Remembery() {}

TObject<IReflect>& Remembery::operator () (IReflect& reflect) {
	BaseClass::operator () (reflect);
	if (reflect.IsReflectMethod()) {
		ReflectMethod(RequestNewDatabase)[ScriptMethod = "NewDatabase"];
		ReflectMethod(RequestExecute)[ScriptMethod = "Execute"];
		ReflectMethod(RequestStep)[ScriptMethod = "Step"];
	}

	return *this;
}

TShared<Hive> Remembery::RequestNewDatabase(IScript::Request& request, const String& target, const String& username, const String& password, bool createOnNonExist) {
	// try to create one
	IDatabase::Database* database = databaseFactory.Connect(archive, target, username, password, createOnNonExist);
	if (database != nullptr) {
		TShared<Hive> hive = TShared<Hive>::From(new Hive(databaseFactory, database));
		hive->SetWarpIndex(bridgeSunset.GetKernel().GetCurrentWarpIndex());
		bridgeSunset.GetKernel().YieldCurrentWarp();
		
		return hive;
	} else {
		request.Error("Remembery::CreateDatabase(target, username, password) : invalid target, username or password.");
		return nullptr;
	}
}

TShared<Honey> Remembery::RequestExecute(IScript::Request& request, IScript::Delegate<Hive> hive, const String& sql, HoneyData& honeyData) {
	CHECK_REFERENCES_NONE();
	CHECK_DELEGATE(hive);

	bridgeSunset.GetKernel().YieldCurrentWarp();
	return hive->Execute(sql, honeyData);
}

void Remembery::RequestStep(IScript::Request& request, IScript::Delegate<Honey> honey, uint32_t count) {
	CHECK_REFERENCES_NONE();
	CHECK_DELEGATE(honey);
	bridgeSunset.GetKernel().YieldCurrentWarp();

	request.DoLock();
	request << beginarray;
	for (uint32_t i = 0; i < count || count == 0; i++) {
		if (!honey->Step()) break;

		honey->WriteLine(request);
	}
	request << endarray;
	request.UnLock();
}
