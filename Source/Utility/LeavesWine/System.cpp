#include "System.h"
#include "../LeavesFlute/LeavesFlute.h"
#include "ImGui/imgui.h"
#include <iterator>

using namespace PaintsNow;
using namespace PaintsNow::NsLeavesWine;
using namespace PaintsNow::NsLeavesFlute;

System::System() : statWindowDuration(500) {}

System::WarpStat::WarpStat(Kernel& k, int& stat) : taskPerFrame(0), kernel(k), expanded(true), lastClockStamp(0), statWindowDuration(stat), historyOffset(0), maxHistory(1.0f) {
	critical.store(0, std::memory_order_relaxed);
	memset(history, 0, sizeof(history));
}

System::WarpStat::~WarpStat() {}

static inline bool SortActiveWarpTinies(const std::pair<WarpTiny*, System::WarpStat::Record>& lhs, const std::pair<WarpTiny*, System::WarpStat::Record>& rhs) {
	return rhs.second.count < lhs.second.count;
}

void System::WarpStat::Execute(void* context) {
	uint32_t warpIndex = kernel.GetCurrentWarpIndex();

	Kernel::SubTaskQueue& taskQueue = kernel.taskQueueGrid[warpIndex];
	const std::vector<TaskQueue::RingBuffer>& ringBuffers = taskQueue.GetRingBuffers();

	for (size_t k = 0; k < ringBuffers.size(); k++) {
		const TaskQueue::RingBuffer& ringBuffer = ringBuffers[k];
		for (TaskQueue::RingBuffer::Iterator p = ringBuffer.Begin(); p != ringBuffer.End(); ++p) {
			const std::pair<ITask*, void*>& task = (*p.p)[p.it];
			WarpTiny* warpTiny = reinterpret_cast<WarpTiny*>(task.second);
			if (warpTiny != nullptr && warpTiny != this) {
				Record& record = activeWarpTinies[warpTiny];
				if (record.count++ == 0) {
					record.unique = warpTiny->GetUnique();
				}
			}
		}
	}

	// Update last result
	int64_t timeStamp = ITimer::GetSystemClock();
	if (timeStamp - lastClockStamp > statWindowDuration) {
		std::vector<std::pair<WarpTiny*, WarpStat::Record> > sortedResults;
		std::copy(activeWarpTinies.begin(), activeWarpTinies.end(), std::back_inserter(sortedResults));
		std::sort(sortedResults.begin(), sortedResults.end(), SortActiveWarpTinies);

		SpinLock(critical);
		std::swap(sortedResults, lastActiveWarpTinies);
		SpinUnLock(critical);

		activeWarpTinies.clear();
		lastClockStamp = timeStamp;

		uint32_t total = 0;
		for (size_t k = 0; k < sortedResults.size(); k++) {
			total += sortedResults[k].second.count;
		}

		historyOffset = (historyOffset + 1) % HISTORY_LENGTH;
		float oldValue = history[historyOffset];
		history[historyOffset] = (float)total;
		maxHistory = Max(maxHistory, (float)total);

		if (oldValue >= maxHistory) {
			float newMax = 0;
			for (size_t n = 0; n < HISTORY_LENGTH; n++) {
				newMax = Max(history[n], newMax);
			}
			maxHistory = newMax;
		}
	}
}

void System::WarpStat::Abort(void* context) {

}

System::WarpStat::Record::Record() : count(0) {}

void System::TickRender(LeavesFlute& leavesFlute) {
	if (ImGui::Begin("System", &show)) {
		Kernel& kernel = leavesFlute.GetKernel();
		uint32_t warpCount = kernel.GetWarpCount();
		if (warpStats.size() != warpCount) {
			warpStats.resize(warpCount);
			for (size_t i = 0; i < warpCount; i++) {
				WarpStat* w = new WarpStat(std::ref(kernel), statWindowDuration);
				w->SetWarpIndex((uint32_t)i);
				warpStats[i] = TShared<WarpStat>::From(w);
			}
		}

		ImGui::SliderInt("Capture Window: (ms)", &statWindowDuration, 1, 5000);

		float maxHistory = 0;
		for (size_t m = 0; m < warpCount; m++) {
			TShared<WarpStat>& stat = warpStats[m];
			maxHistory = Max(maxHistory, stat->maxHistory);
		}

		ImGui::Text("Warp Usage: (Max %d)", (int)maxHistory);
		ImGui::Columns(4);
		for (size_t k = 0; k < warpCount; k++) {
			TShared<WarpStat>& stat = warpStats[k];
			kernel.QueueRoutine(stat(), stat());
			char overlay[32];
			sprintf(overlay, "Warp %d", (int)k);
			ImGui::PlotLines("", stat->history, IM_ARRAYSIZE(stat->history), stat->historyOffset, overlay, 0.0f, maxHistory, ImVec2(ImGui::GetColumnWidth(), 50.0f));
			ImGui::NextColumn();
		}
		ImGui::Columns(1);

		ImGui::Text("Warp Task Ranking:");
		ImGui::Columns(3);
		ImGui::TextColored(ImVec4(1, 1, 0, 1), "WarpIndex");
		ImGui::NextColumn();
		ImGui::TextColored(ImVec4(1, 1, 0, 1), "Object");
		ImGui::NextColumn();
		ImGui::TextColored(ImVec4(1, 1, 0, 1), "Count");
		ImGui::NextColumn();

		if (!inited) {
			ImGui::SetColumnWidth(0, 100.0f);
			ImGui::SetColumnWidth(1, 320.0f);
			ImGui::SetColumnWidth(2, 300.0f);
			inited = true;
		}

		for (size_t i = 0; i < warpCount; i++) {
			TShared<WarpStat>& stat = warpStats[i];
			std::vector<std::pair<WarpTiny*, WarpStat::Record> > sortedResults;
			SpinLock(stat->critical);
			std::swap(sortedResults, stat->lastActiveWarpTinies);
			SpinUnLock(stat->critical);

			for (size_t m = 0; m < sortedResults.size(); m++) {
				const std::pair<WarpTiny*, WarpStat::Record>& t = sortedResults[m];

				if (t.first != stat()) {
					ImGui::Text("%d", (int)i);
					ImGui::NextColumn();
					ImGui::Text("%p : %s", t.first, RemoveNamespace(t.second.unique->typeName).c_str());
					ImGui::NextColumn();
					ImGui::Text("%d", t.second.count);
					ImGui::NextColumn();
				}
			}

			SpinLock(stat->critical);
			if (stat->lastActiveWarpTinies.empty()) {
				std::swap(sortedResults, stat->lastActiveWarpTinies);
			}
			SpinUnLock(stat->critical);
		}

		ImGui::Columns(1);
	}
	
	ImGui::End();
}