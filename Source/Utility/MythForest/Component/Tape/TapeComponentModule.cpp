#include "TapeComponentModule.h"
#include "TapeComponent.h"
#include "../../../SnowyStream/File.h"
#include "../../../SnowyStream/Resource/StreamResource.h"

using namespace PaintsNow;
using namespace PaintsNow::NsMythForest;
using namespace PaintsNow::NsSnowyStream;

TapeComponentModule::TapeComponentModule(Engine& engine) : BaseClass(engine) {}

TObject<IReflect>& TapeComponentModule::operator () (IReflect& reflect) {
	BaseClass::operator () (reflect);

	if (reflect.IsReflectMethod()) {
		ReflectMethod(RequestNew)[ScriptMethod = "New"];
	}

	return *this;
}

TShared<TapeComponent> TapeComponentModule::RequestNew(IScript::Request& request, IScript::Delegate<SharedTiny> streamHolder) {
	CHECK_REFERENCES_NONE();
	CHECK_DELEGATE(streamHolder);
	TShared<TapeComponent> tapeComponent;

	File* file = streamHolder->QueryInterface(UniqueType<File>());
	if (file != nullptr) {
		assert(file->GetStream() != nullptr);
		tapeComponent = TShared<TapeComponent>::From(allocator->New(std::ref(*file->GetStream()), file));
	} else {
		StreamResource* res = streamHolder->QueryInterface(UniqueType<StreamResource>());
		if (res != nullptr) {
			tapeComponent = TShared<TapeComponent>::From(allocator->New(std::ref(res->GetStream()), res));
		}
	}

	if (tapeComponent) {
		tapeComponent->SetWarpIndex(engine.GetKernel().GetCurrentWarpIndex());
	}

	return tapeComponent;
}

std::pair<int64_t, String> TapeComponentModule::RequestRead(IScript::Request& request, IScript::Delegate<TapeComponent> tapeComponent) {
	CHECK_REFERENCES_NONE();
	CHECK_DELEGATE(tapeComponent);
	engine.GetKernel().YieldCurrentWarp();

	return tapeComponent->Read();
}

bool TapeComponentModule::RequestWrite(IScript::Request& request, IScript::Delegate<TapeComponent> tapeComponent, int64_t seq, const String& data) {
	CHECK_REFERENCES_NONE();
	CHECK_DELEGATE(tapeComponent);
	engine.GetKernel().YieldCurrentWarp();

	return tapeComponent->Write(seq, data);
}

bool TapeComponentModule::RequestSeek(IScript::Request& request, IScript::Delegate<TapeComponent> tapeComponent, int64_t seq) {
	CHECK_REFERENCES_NONE();
	CHECK_DELEGATE(tapeComponent);
	engine.GetKernel().YieldCurrentWarp();

	return tapeComponent->Seek(seq);
}