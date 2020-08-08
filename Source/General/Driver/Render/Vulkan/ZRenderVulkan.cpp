#ifdef _MSC_VER
#pragma warning(disable:4786)
#endif

#include "../../../Interface/Interfaces.h"
#include "../../../Interface/IShader.h"
#include "../../../../Core/Interface/IMemory.h"
#include "../../../../Core/Template/TQueue.h"
#include "ZRenderVulkan.h"
#include <cstdio>
#include <vector>
#include <iterator>
#include <sstream>

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

using namespace PaintsNow;

IRender::Device* ZRenderVulkan::CreateDevice(const String& description) {
	return nullptr;
}

void ZRenderVulkan::SetDeviceResolution(IRender::Device* device, const Int2& resolution) {}
Int2 ZRenderVulkan::GetDeviceResolution(IRender::Device* device) { return Int2(0, 0); }
void ZRenderVulkan::DeleteDevice(IRender::Device* device) {}
IRender::Queue* ZRenderVulkan::CreateQueue(Device* device, bool shared) { return nullptr; }
IRender::Device* ZRenderVulkan::GetQueueDevice(Queue* queue) { return nullptr; }
void ZRenderVulkan::PresentQueues(Queue** queues, uint32_t count, PresentOption option) {}

bool ZRenderVulkan::SupportParallelPresent(Device* device) {
	return false;
}

bool ZRenderVulkan::IsQueueEmpty(Queue* queue) { return true; }
void ZRenderVulkan::MergeQueue(Queue* target, Queue* source) {}
void ZRenderVulkan::YieldQueue(Queue* queue) {}
void ZRenderVulkan::DeleteQueue(Queue* queue) {}

// Resource
IRender::Resource* ZRenderVulkan::CreateResource(Queue* queue, Resource::Type resourceType) { return nullptr; }
void ZRenderVulkan::UploadResource(Queue* queue, Resource* resource, Resource::Description* description) {}
void ZRenderVulkan::RequestDownloadResource(Queue* queue, Resource* resource, Resource::Description* description) {}
void ZRenderVulkan::CompleteDownloadResource(Queue* queue, Resource* resource) {}
void ZRenderVulkan::ExecuteResource(Queue* queue, Resource* resource) {}
void ZRenderVulkan::SwapResource(Queue* queue, Resource* lhs, Resource* rhs) {}
void ZRenderVulkan::DeleteResource(Queue* queue, Resource* resource) {}

ZRenderVulkan::ZRenderVulkan() {
/*
	VkApplicationInfo appInfo = {};
	appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
	appInfo.pNext = nullptr;
	appInfo.pApplicationName = "PaintsNow.Net";
	appInfo.applicationVersion = VK_MAKE_VERSION(0, 0, 0);
	appInfo.pEngineName = "PaintsNow";
	appInfo.engineVersion = VK_MAKE_VERSION(0, 0, 0);
	appInfo.apiVersion = VK_API_VERSION_1_1;

	VkInstanceCreateInfo createInfo = {};
	createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
	createInfo.pApplicationInfo = &appInfo;

	uint32_t glfwExtensionCount = 0;
	const char** glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);
	createInfo.enabledExtensionCount = glfwExtensionCount;
	createInfo.ppEnabledExtensionNames = glfwExtensions;
	createInfo.enabledLayerCount = 0;

	if (vkCreateInstance(&createInfo, nullptr, &instance) != VK_SUCCESS) {
		fprintf(stderr, "Create vulkan instance failed!");
		exit(0);
	}

	uint32_t deviceCount = 0;
	vkEnumeratePhysicalDevices(instance, &deviceCount, nullptr);

	if (deviceCount == 0) {
		fprintf(stderr, "No supported vulkan device!");
		exit(0);
	}

	std::vector<VkPhysicalDevice> devices(deviceCount);
	vkEnumeratePhysicalDevices(instance, &deviceCount, devices.data());

	// TODO: select first device now
	uint32_t queueFamilyCount = 0;
	vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, nullptr);

	std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
	vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, queueFamilies.data());*/
}

ZRenderVulkan::~ZRenderVulkan() {
	// vkDestroyInstance(instance, nullptr);
}
