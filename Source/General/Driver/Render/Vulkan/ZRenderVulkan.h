// ZRenderVulkan.h -- Vulkan Render API provider
// By PaintDream (paintdream@paintdream.com)
// 2014-12-1
//

#ifndef __ZRENDERVULKAN_H__
#define __ZRENDERVULKAN_H__

#include "../../../../Core/PaintsNow.h"
#include "../../../Interface/IRender.h"
#include "../../../../Core/Interface/IThread.h"
#include "../../../../Core/Interface/IReflect.h"
#include "../../../../Core/Template/TQueue.h"

typedef struct VkInstance_T* VkInstance;
typedef struct VkPhysicalDevice_T* VkPhysicalDevice;
struct VkAllocationCallbacks;
struct GLFWwindow;

namespace PaintsNow {
	class ZRenderVulkan final : public IRender {
	public:
		ZRenderVulkan(GLFWwindow* window);
		virtual ~ZRenderVulkan();

		virtual std::vector<String> EnumerateDevices() override;
		virtual Device* CreateDevice(const String& description) override;
		virtual Int2 GetDeviceResolution(Device* device) override;
		virtual void SetDeviceResolution(Device* device, const Int2& resolution) override;
		virtual void NextDeviceFrame(Device* device) override;
		virtual void DeleteDevice(Device* device) override;

		virtual void PresentQueues(Queue** queues, uint32_t count, PresentOption option) override;
		virtual bool SupportParallelPresent(Device* device) override;

		// Queue
		virtual Queue* CreateQueue(Device* device, uint32_t flag) override;
		virtual Device* GetQueueDevice(Queue* queue) override;
		virtual void DeleteQueue(Queue* queue) override;
		virtual void FlushQueue(Queue* queue) override;
		virtual bool IsQueueEmpty(Queue* queue) override;

		// Resource
		virtual Resource* CreateResource(Device* device, Resource::Type resourceType) override;
		virtual void UploadResource(Queue* queue, Resource* resource, Resource::Description* description) override;
		virtual void AcquireResource(Queue* queue, Resource* resource) override;
		virtual void ReleaseResource(Queue* queue, Resource* resource) override;
		virtual void RequestDownloadResource(Queue* queue, Resource* resource, Resource::Description* description) override;
		virtual void CompleteDownloadResource(Queue* queue, Resource* resource) override;
		virtual void ExecuteResource(Queue* queue, Resource* resource) override;
		virtual void SwapResource(Queue* queue, Resource* lhs, Resource* rhs) override;
		virtual void DeleteResource(Queue* queue, Resource* resource) override;

	protected:
		GLFWwindow* window;
		VkInstance instance;
		size_t surface;
		uint64_t frameIndex;
		VkAllocationCallbacks* allocator;
		std::vector<VkPhysicalDevice> gpus;

		#ifdef _DEBUG
		size_t debugCallback;
		#endif
	};
}


#endif // __ZRENDERVULKAN_H__
