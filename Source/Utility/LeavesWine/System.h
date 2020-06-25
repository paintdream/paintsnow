// System.h
// PaintDream (paintdream@paintdream.com)
// 2019-11-1
//

#ifndef __SYSTEM_H__
#define __SYSTEM_H__

#include "IWidget.h"
#include "../../Core/System/Kernel.h"

namespace PaintsNow {
	namespace NsLeavesWine {
		class System : public IWidget {
		public:
			System();
			virtual void TickRender(NsLeavesFlute::LeavesFlute& leavesFlute) override;

			struct WarpStat : public WarpTiny, public TaskRepeat {
				WarpStat(Kernel& kernel, int& stat);
				virtual ~WarpStat();
				virtual void Execute(void* context) override;
				virtual void Abort(void* context) override;
			
				Kernel& kernel;
				int& statWindowDuration;
				std::atomic<uint32_t> taskPerFrame;
				std::atomic<uint32_t> critical;
				int64_t lastClockStamp;
				bool expanded;
				bool reserved[3];

				struct Record {
					Record();
					uint32_t count;
					Unique unique;
				};

				std::map<WarpTiny*, Record> activeWarpTinies;
				std::vector<std::pair<WarpTiny*, Record> > lastActiveWarpTinies;
				enum { HISTORY_LENGTH = 64 };
				uint32_t historyOffset;
				float maxHistory;
				float history[HISTORY_LENGTH];
			};

			std::vector<TShared<WarpStat> > warpStats;
			int statWindowDuration;
		};
	}
}

#endif // __SYSTEM_H__