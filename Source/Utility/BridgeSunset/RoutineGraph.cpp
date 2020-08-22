#include "RoutineGraph.h"

using namespace PaintsNow;

RoutineGraph::RoutineGraph() : taskGraph(nullptr) {}

uint32_t RoutineGraph::Insert(Kernel& kernel, WarpTiny* host, ITask* task) {
	assert(!(Flag() & TINY_ACTIVATED));
	if (taskGraph == nullptr) {
		taskGraph = new TaskGraph(kernel);
	}

	return taskGraph->Insert(host, task);
}

void RoutineGraph::Next(uint32_t from, uint32_t to) {
	assert(taskGraph != nullptr);
	assert(!(Flag() & TINY_ACTIVATED));

	taskGraph->Next(from, to);
}

void RoutineGraph::Commit() {
	assert(taskGraph != nullptr);
	assert(!(Flag() & TINY_ACTIVATED));

	taskGraph->Commit(Wrap(this, &RoutineGraph::AllFinished));
	ReferenceObject();
}

void RoutineGraph::AllFinished() {
	taskGraph = nullptr;
	ReleaseObject();
}