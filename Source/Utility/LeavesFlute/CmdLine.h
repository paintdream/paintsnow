// CmdLine.h
// By PaintDream (paintdream@paintdream.com)
// 2016-1-1
//

#ifndef __CMDLINE_H__
#define __CMDLINE_H__

#include "../../Core/PaintsNow.h"
#include "../../Core/Interface/IType.h"
#include <string>
#include <map>
#include <list>

namespace PaintsNow {
	namespace NsLeavesFlute {
		class CmdLine {
		public:
			virtual ~CmdLine();

			void Process(int argc, char* argv[]);
			struct Option {
				String name;
				String param;
			};
			const std::map<String, Option>& GetFactoryMap() const;
			const std::map<String, Option>& GetConfigMap() const;
			const std::list<Option>& GetModuleList() const;
			const std::list<String>& GetPackageList() const;

		private:
			void ProcessOne(const String& str);
			void ProcessOption(const String& str);
			void ProcessPackage(const String& str);
			// void ProcessFactory(const String& str);
			// void ProcessParam(const String& str);

			std::map<String, Option> factoryMap;
			std::map<String, Option> configMap;
			std::list<Option> moduleList;
			std::list<String> packageList;
		};
	}
}


#endif // __CMDLINE_H__