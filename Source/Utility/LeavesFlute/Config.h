// Config.h
// By PaintDream (paintdream@paintdream.com)
// 2016-1-1
//

#ifndef __CONFIG_H__
#define __CONFIG_H__

#include "../../Core/PaintsNow.h"
#include "../../Core/Template/TFactory.h"
#include "LeavesApi.h"
#include <string>
#include <map>
#include <list>

namespace PaintsNow {
	namespace NsLeavesFlute {
		class Config : public LeavesApi {
		public:
			virtual ~Config();
			virtual void RegisterFactory(const String& factoryEntry, const String& name, const TWrapper<void*, const String&>* factoryBase);
			virtual void UnregisterFactory(const String& factoryEntry, const String& name);
			virtual void QueryFactory(const String& factoryEntry, const TWrapper<void, const String&, const TWrapper<void*, const String&>*>& callback);
			virtual void RegisterRuntimeHook(const TWrapper<void, NsLeavesFlute::LeavesFlute*, RUNTIME_STATE>& proc);
			virtual void WriteString(String& target, const String& source) const;

			void PostRuntimeState(NsLeavesFlute::LeavesFlute* leavesFlute, RUNTIME_STATE state);

			struct Entry {
				String name;
				const TWrapper<void*, const String&>* factoryBase;
			};

			const std::list<Entry>& GetEntry(const String& factoryEntry) const;

		private:
			std::map<String, std::list<Entry> > mapFactories;
			std::vector<TWrapper<void, NsLeavesFlute::LeavesFlute*, RUNTIME_STATE> > runtimeHooks;
		};
	}
}


#endif // __CONFIG_H__
