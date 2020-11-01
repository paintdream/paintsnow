#include "ZArchiveVirtual.h"
#include "../../../../Core/System/MemoryStream.h"

using namespace PaintsNow;

const String& ZArchiveVirtual::GetRootPath() const {
	static const String empty = "";
	return empty;
}

void ZArchiveVirtual::SetRootPath(const String& path) {

}

ZArchiveVirtual::ZArchiveVirtual() {}

ZArchiveVirtual::~ZArchiveVirtual() {}

IStreamBase* ZArchiveVirtual::Open(const String& uri, bool write, size_t& length, uint64_t* lastModifiedTime) {
	return nullptr;
}

void ZArchiveVirtual::Query(const String& uri, const TWrapper<void, bool, const String&>& wrapper) const {
}

bool ZArchiveVirtual::IsReadOnly() const {
	return true;
}

bool ZArchiveVirtual::Delete(const String& uri) {
	assert(false); // do not support this operation
	return false;
}
