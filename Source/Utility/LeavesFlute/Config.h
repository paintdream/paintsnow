// Config.h
// PaintDream (paintdream@paintdream.com)
// 2016-1-1
//

#pragma once
#include "../../Core/PaintsNow.h"
#include "LeavesApi.h"
#include <string>
#include <map>
#include <list>

namespace PaintsNow {
	class Config : public LeavesApi {
	public:
		~Config() override;
		void RegisterFactory(const String& factoryEntry, const String& name, const TWrapper<IDevice*>& factoryBase) override;
		void UnregisterFactory(const String& factoryEntry, const String& name) override;

		struct Entry {
			String name;
			TWrapper<IDevice*> factoryBase;
		};

		const std::list<Entry>& GetEntry(const String& factoryEntry) const;

	private:
		std::map<String, std::list<Entry> > mapFactories;
	};
}
