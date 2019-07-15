#include "Config.h"

using namespace PaintsNow;
using namespace PaintsNow::NsLeavesFlute;

Config::~Config() {}

void Config::RegisterFactory(const String& factoryEntry, const String& name, const TWrapper<void*, const String&>* factoryBase) {
	Entry entry;
	entry.name = name;
	entry.factoryBase = factoryBase;

	mapFactories[factoryEntry].emplace_back(std::move(entry));
	// printf("Register new factory<%s> : %s\n", factoryEntry.c_str(), name.c_str());
}


void Config::UnregisterFactory(const String& factoryEntry, const String& name) {
	std::list<Entry> entries = mapFactories[factoryEntry];
	for (std::list<Entry>::iterator p = entries.begin(); p != entries.end(); ++p) {
		if ((*p).name == name) {
			entries.erase(p);
			break;
		}
	}
}

void Config::QueryFactory(const String& factoryEntry, const TWrapper<void, const String&, const TWrapper<void*, const String&>*>& callback) {
	std::list<Entry> entries = mapFactories[factoryEntry];
	for (std::list<Entry>::iterator p = entries.begin(); p != entries.end(); ++p) {
		callback((*p).name, (*p).factoryBase);
	}
}

const std::list<Config::Entry>& Config::GetEntry(const String& factoryEntry) const {
	const std::map<String, std::list<Entry> >::const_iterator p = mapFactories.find(factoryEntry);
	if (p != mapFactories.end()) {
		return p->second;
	} else {
		static const std::list<Config::Entry> dummy;
		return dummy;
	}
}

void Config::RegisterRuntimeHook(const TWrapper<void, NsLeavesFlute::LeavesFlute*, RUNTIME_STATE>& proc) {
	runtimeHooks.emplace_back(proc);
}

void Config::WriteString(String& target, const String& source) const {
	target.assign(source.c_str(), source.size());
}

void Config::PostRuntimeState(NsLeavesFlute::LeavesFlute* leavesFlute, RUNTIME_STATE state) {
	for (size_t i = 0; i < runtimeHooks.size(); i++) {
		runtimeHooks[i](leavesFlute, state);
	}
}