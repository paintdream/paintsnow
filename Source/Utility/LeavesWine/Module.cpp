#include "Module.h"
#include "../LeavesFlute/LeavesFlute.h"
#include "ImGui/imgui.h"
#include <sstream>

using namespace PaintsNow;
using namespace PaintsNow::NsLeavesWine;

void Module::TickRender(NsLeavesFlute::LeavesFlute& leavesFlute) {
	if (ImGui::Begin("Module", &show)) {
		RenderObject(leavesFlute);
	}

	ImGui::End();
}
