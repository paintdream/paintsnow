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

struct VulkanDeviceImpl : public IRender::Device {
	VulkanDeviceImpl(VkPhysicalDevice dev) : device(dev), resolution(0, 0) {}

	VkPhysicalDevice device;
	Int2 resolution;
};

std::vector<String> ZRenderVulkan::EnumerateDevices() {
	std::vector<String> devices;

	uint32_t gpuCount;
	VkResult err = vkEnumeratePhysicalDevices(instance, &gpuCount, NULL);
	if (err != VK_SUCCESS || gpuCount == 0) {
		return devices;
	}

	std::vector<VkPhysicalDevice> gpus(gpuCount);
	err = vkEnumeratePhysicalDevices(instance, &gpuCount, &gpus[0]);
	devices.resize(gpuCount);

	for (size_t i = 0; i < gpus.size(); i++) {
		VkPhysicalDeviceProperties prop;
		vkGetPhysicalDeviceProperties(gpus[i], &prop);
		devices.emplace_back(String(prop.deviceName) + " [" + std::to_string(i) + "]");
	}

	return devices;
}

IRender::Device* ZRenderVulkan::CreateDevice(const String& description) {
	uint32_t gpuCount;
	VkResult err = vkEnumeratePhysicalDevices(instance, &gpuCount, NULL);
	if (err != VK_SUCCESS || gpuCount == 0) {
		return nullptr;
	}

	std::vector<VkPhysicalDevice> gpus(gpuCount);
	err = vkEnumeratePhysicalDevices(instance, &gpuCount, &gpus[0]);

	for (size_t i = 0; i < gpus.size(); i++) {
		VkPhysicalDeviceProperties prop;
		vkGetPhysicalDeviceProperties(gpus[i], &prop);
		String name = String(prop.deviceName) + " [" + std::to_string(i) + "]";
		if (name == description) {
			return new VulkanDeviceImpl(gpus[i]);
		}
	}

	return nullptr;
}

void ZRenderVulkan::DeleteDevice(IRender::Device* device) {
	VulkanDeviceImpl* impl = static_cast<VulkanDeviceImpl*>(device);
	delete impl;
}

void ZRenderVulkan::SetDeviceResolution(IRender::Device* device, const Int2& resolution) {
	VulkanDeviceImpl* impl = static_cast<VulkanDeviceImpl*>(device);
	impl->resolution = resolution;
}

Int2 ZRenderVulkan::GetDeviceResolution(IRender::Device* device) {
	VulkanDeviceImpl* impl = static_cast<VulkanDeviceImpl*>(device);
	return impl->resolution;
}

IRender::Queue* ZRenderVulkan::CreateQueue(Device* device, uint32_t flag) { return nullptr; }
IRender::Device* ZRenderVulkan::GetQueueDevice(Queue* queue) { return nullptr; }
void ZRenderVulkan::PresentQueues(Queue** queues, uint32_t count, PresentOption option) {}

bool ZRenderVulkan::SupportParallelPresent(Device* device) {
	return false;
}

bool ZRenderVulkan::IsQueueEmpty(Queue* queue) { return true; }
void ZRenderVulkan::MergeQueue(Queue* target, Queue* source) {}
void ZRenderVulkan::FlushQueue(Queue* queue) {}
void ZRenderVulkan::DeleteQueue(Queue* queue) {}

// Resource
IRender::Resource* ZRenderVulkan::CreateResource(Device* device, Resource::Type resourceType) { return nullptr; }
void ZRenderVulkan::UploadResource(Queue* queue, Resource* resource, Resource::Description* description) {}
void ZRenderVulkan::AcquireResource(Queue* queue, Resource* resource) {}
void ZRenderVulkan::ReleaseResource(Queue* queue, Resource* resource) {}
void ZRenderVulkan::RequestDownloadResource(Queue* queue, Resource* resource, Resource::Description* description) {}
void ZRenderVulkan::CompleteDownloadResource(Queue* queue, Resource* resource) {}
void ZRenderVulkan::ExecuteResource(Queue* queue, Resource* resource) {}
void ZRenderVulkan::SwapResource(Queue* queue, Resource* lhs, Resource* rhs) {}
void ZRenderVulkan::DeleteResource(Queue* queue, Resource* resource) {}

#ifdef _DEBUG
static VKAPI_ATTR VkBool32 VKAPI_CALL DebugReportCallback(VkDebugReportFlagsEXT flags, VkDebugReportObjectTypeEXT objectType, uint64_t object, size_t location, int32_t messageCode, const char* pLayerPrefix, const char* pMessage, void* pUserData) {
	fprintf(stderr, "Vulkan debug: %d - %s\n", objectType, pMessage);
	return VK_FALSE;
}
#endif

ZRenderVulkan::ZRenderVulkan() : allocator(nullptr) {
	VkApplicationInfo appInfo = {};
	appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
	appInfo.pNext = nullptr;
	appInfo.pApplicationName = "PaintsNow.Net";
	appInfo.applicationVersion = VK_MAKE_VERSION(0, 0, 0);
	appInfo.pEngineName = "PaintsNow";
	appInfo.engineVersion = VK_MAKE_VERSION(0, 0, 0);
	appInfo.apiVersion = VK_API_VERSION_1_1;

	uint32_t extensionsCount = 0;
	const char** extensions = glfwGetRequiredInstanceExtensions(&extensionsCount);

	VkInstanceCreateInfo createInfo = {};
	createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
	createInfo.enabledExtensionCount = extensionsCount;
	createInfo.ppEnabledExtensionNames = extensions;

#ifdef _DEBUG 
	// Enabling multiple validation layers grouped as LunarG standard validation
	const char* layers[] = { "VK_LAYER_LUNARG_standard_validation" };
	createInfo.enabledLayerCount = 1;
	createInfo.ppEnabledLayerNames = layers;

	// Enable debug report extension (we need additional storage, so we duplicate the user array to add our new extension to it)
	std::vector<const char*> extensions_ext(extensionsCount + 1);
	memcpy(&extensions_ext[0], extensions, extensionsCount * sizeof(const char*));
	extensions_ext[extensionsCount] = "VK_EXT_debug_report";
	createInfo.enabledExtensionCount = extensionsCount + 1;
	createInfo.ppEnabledExtensionNames = &extensions_ext[0];

	// Create Vulkan Instance
	VkResult err = vkCreateInstance(&createInfo, allocator, &instance);
	if (err != VK_SUCCESS) {
		fprintf(stderr, "Unable to create vulkan instance (debug).\n");
		exit(0);
	}

	// Get the function pointer (required for any extensions)
	auto vkCreateDebugReportCallbackEXT = (PFN_vkCreateDebugReportCallbackEXT)vkGetInstanceProcAddr(instance, "vkCreateDebugReportCallbackEXT");
	assert(vkCreateDebugReportCallbackEXT != NULL);

	// Setup the debug report callback
	VkDebugReportCallbackCreateInfoEXT debugReport = {};
	debugReport.sType = VK_STRUCTURE_TYPE_DEBUG_REPORT_CALLBACK_CREATE_INFO_EXT;
	debugReport.flags = VK_DEBUG_REPORT_ERROR_BIT_EXT | VK_DEBUG_REPORT_WARNING_BIT_EXT | VK_DEBUG_REPORT_PERFORMANCE_WARNING_BIT_EXT;
	debugReport.pfnCallback = DebugReportCallback;
	debugReport.pUserData = NULL;
	err = vkCreateDebugReportCallbackEXT(instance, &debugReport, allocator, (VkDebugReportCallbackEXT*)&debugCallback);
#else
	// Create Vulkan Instance without any debug feature
	VkResult err = vkCreateInstance(&createInfo, allocator, &instance);
	if (err != VK_SUCCESS) {
		fprintf(stderr, "Unable to create vulkan instance (release).\n");
		exit(0);
	}
#endif
}

ZRenderVulkan::~ZRenderVulkan() {
#ifdef _DEBUG
	PFN_vkDestroyDebugReportCallbackEXT vkDestroyDebugReportCallbackEXT = (PFN_vkDestroyDebugReportCallbackEXT)vkGetInstanceProcAddr(instance, "vkDestroyDebugReportCallbackEXT");
	vkDestroyDebugReportCallbackEXT(instance, (VkDebugReportCallbackEXT)debugCallback, allocator);
#endif

	vkDestroyInstance(instance, nullptr);
}
