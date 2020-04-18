// RoutineGraph.h
// PaintDream (paintdream@paintdream.com)
// 2020-4-18
//

#ifndef __ROUTINEGRAPH_H__
#define __ROUTINEGRAPH_H__

#include "../../Core/System/TaskGraph.h"

namespace PaintsNow {
	namespace NsBridgeSunset {
		class RoutineGraph : public TReflected<RoutineGraph, WarpTiny> {
		public:
			RoutineGraph();
			uint32_t Insert(Kernel& kernel, WarpTiny* host, ITask* task);
			void Next(uint32_t from, uint32_t to);
			void Commit();

		protected:
			void AllFinished();
			TaskGraph* taskGraph;
		};
	}
}

#endif // __ROUTINEGRAPH_H__