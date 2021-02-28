#include "IModule.h"
#include "../../../LeavesFlute/LeavesFlute.h"
#include "Core/imgui.h"
#include <sstream>

using namespace PaintsNow;

void IModule::TickRender(LeavesFlute& leavesFlute) {
	if (ImGui::Begin("Module", &show)) {
		RenderObject(leavesFlute);
	}

	ImGui::End();
}
