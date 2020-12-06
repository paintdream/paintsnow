#include "SurfaceComponent.h"

using namespace PaintsNow;

SurfaceComponent::SurfaceComponent() {}

uint32_t SurfaceComponent::CollectDrawCalls(std::vector<OutputRenderData, DrawCallAllocator>& outputDrawCalls, const InputRenderData& inputRenderData, BytesCache& bytesCache) {
	return 0;
}
