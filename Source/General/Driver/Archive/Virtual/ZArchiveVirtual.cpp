#include "ZArchiveVirtual.h"
#include "../../../../Core/System/MemoryStream.h"

using namespace PaintsNow;

String ZArchiveVirtual::GetFullPath(const String& path) const {
	return path;
}

bool ZArchiveVirtual::Mount(const String& prefix, IArchive* archive) {
	assert(archive != nullptr);
	// check if exists?
	// allow multiple mounts!
	/*
	for (size_t i = 0; i < mountInfos.size(); i++) {
		MountInfo& info = mountInfos[i];
		if (info.prefix == prefix) {
			info.archive = archive;

			// always success
			return true;
		}
	}*/

	MountInfo info;
	info.archive = archive;
	info.prefix = prefix;

	mountInfos.emplace_back(std::move(info));
	return true;
}

bool ZArchiveVirtual::Unmount(const String& prefix, IArchive* archive) {
	for (size_t i = 0; i < mountInfos.size(); i++) {
		MountInfo& info = mountInfos[i];
		if (info.archive == archive && info.prefix == prefix) {
			mountInfos.erase(mountInfos.begin() + i);
			return true;
		}
	}

	return false;
}

ZArchiveVirtual::ZArchiveVirtual() {}

ZArchiveVirtual::~ZArchiveVirtual() {}

IStreamBase* ZArchiveVirtual::Open(const String& uri, bool write, size_t& length, uint64_t* lastModifiedTime) {
	for (size_t i = 0; i < mountInfos.size(); i++) {
		MountInfo& info = mountInfos[i];
		if (info.prefix.compare(0, info.prefix.length(), uri) == 0) {
			return info.archive->Open(uri.substr(info.prefix.length()), write, length, lastModifiedTime);
		}
	}

	return nullptr;
}

void ZArchiveVirtual::Query(const String& uri, const TWrapper<void, bool, const String&>& wrapper) const {
	for (size_t i = 0; i < mountInfos.size(); i++) {
		const MountInfo& info = mountInfos[i];
		if (info.prefix.compare(0, info.prefix.length(), uri) == 0) {
			info.archive->Query(uri.substr(info.prefix.length()), wrapper);
		}
	}
}

bool ZArchiveVirtual::IsReadOnly() const {
	for (size_t i = 0; i < mountInfos.size(); i++) {
		const MountInfo& info = mountInfos[i];
		if (!info.archive->IsReadOnly())
			return false;
	}

	return true;
}

bool ZArchiveVirtual::Delete(const String& uri) {
	for (size_t i = 0; i < mountInfos.size(); i++) {
		const MountInfo& info = mountInfos[i];
		if (info.prefix.compare(0, info.prefix.length(), uri) == 0) {
			if (info.archive->Delete(uri.substr(info.prefix.length())))
				return true;
		}
	}

	return false;
}
