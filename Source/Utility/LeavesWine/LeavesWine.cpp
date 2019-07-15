#include "LeavesWine.h"
#include "ImGui/imgui.h"

using namespace PaintsNow;
using namespace PaintsNow::NsLeavesWine;
using namespace PaintsNow::NsLeavesFlute;

LeavesWine::LeavesWine(LeavesFlute*& flute) : leavesFlute(flute) {}

void LeavesWine::AddWidget(IWidget* widget) {
	widgets.emplace_back(widget);
}

void LeavesWine::TickRender() {
	if (leavesFlute != nullptr) {
		for (size_t i = 0; i < widgets.size(); i++) {
			ImGui::SetNextWindowPos(ImVec2(100, 20.0f + 50 * i), ImGuiCond_FirstUseEver);
			ImGui::SetNextWindowSize(ImVec2(720, 280), ImGuiCond_FirstUseEver);
			widgets[i]->TickRender(*leavesFlute);
		}
	}
}