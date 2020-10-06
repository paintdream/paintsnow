// RoutineGraph.h
// PaintDream (paintdream@paintdream.com)
// 2020-4-18
//

#pragma once
#include "../../Core/System/TaskGraph.h"

namespace PaintsNow {
	class RoutineGraph : public TReflected<RoutineGraph, WarpTiny> {
	public:
		RoutineGraph();
		~RoutineGraph() override;
		uint32_t Insert(Kernel& kernel, WarpTiny* host, ITask* task);
		void Next(uint32_t from, uint32_t to);
		void Commit();

	protected:
		void AllFinished();
		TaskGraph* taskGraph;
	};
}

