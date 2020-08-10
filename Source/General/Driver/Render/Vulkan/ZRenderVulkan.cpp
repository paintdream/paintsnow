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

const int MIN_IMAGE_COUNT = 2;

static void Verify(const char* message, VkResult res) {
	if (res != VK_SUCCESS) {
		fprintf(stderr, "Unable to %s (debug).\n", message);
		exit(0);
	}
}

struct VulkanDeviceImpl : public IRender::Device {
	VulkanDeviceImpl(VkPhysicalDevice phy, VkDevice dev, uint32_t family, VkQueue q) : physicalDevice(phy), device(dev), resolution(0, 0), queueFamily(family), queue(q), swapChain(0) {}

	VkPhysicalDevice physicalDevice;
	VkDevice device;
	VkQueue queue;
	Int2 resolution;
	uint32_t queueFamily;
	VkSwapchainKHR swapChain;

	std::vector<VkImage> backBuffers;
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
		if (name == description || description.empty()) {
			VkPhysicalDevice device = gpus[i];

			uint32_t count;
			vkGetPhysicalDeviceQueueFamilyProperties(device, &count, NULL);
			std::vector<VkQueueFamilyProperties> queues(count);
			if (count == 0) return nullptr;

			uint32_t family = ~(uint32_t)0;
			vkGetPhysicalDeviceQueueFamilyProperties(device, &count, &queues[0]);
			for (uint32_t i = 0; i < count; i++) {
				if (queues[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) {
					family = i;
					break;
				}
			}

			if (family == ~(uint32_t)0) {
				return nullptr;
			}

			int deviceExtensionCount = 1;
			const char* deviceExtensions[] = { "VK_KHR_swapchain" };
			const float queuePriority[] = { 1.0f };
			VkDeviceQueueCreateInfo queueInfo[1] = {};
			queueInfo[0].sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
			queueInfo[0].queueFamilyIndex = family;
			queueInfo[0].queueCount = 1;
			queueInfo[0].pQueuePriorities = queuePriority;
			VkDeviceCreateInfo createInfo = {};
			createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
			createInfo.queueCreateInfoCount = sizeof(queueInfo) / sizeof(queueInfo[0]);
			createInfo.pQueueCreateInfos = queueInfo;
			createInfo.enabledExtensionCount = deviceExtensionCount;
			createInfo.ppEnabledExtensionNames = deviceExtensions;

			VkDevice logicDevice;
			Verify("create device", vkCreateDevice(device, &createInfo, allocator, &logicDevice));
			VkQueue queue;
			vkGetDeviceQueue(logicDevice, family, 0, &queue);

			VkBool32 res;
			vkGetPhysicalDeviceSurfaceSupportKHR(device, family, (VkSurfaceKHR)surface, &res);

			VulkanDeviceImpl* impl = new VulkanDeviceImpl(device, logicDevice, family, queue);

			int w, h;
			glfwGetFramebufferSize(window, &w, &h);
			SetDeviceResolution(impl, Int2(w, h));

			return impl;
		}
	}

	return nullptr;
}

void ZRenderVulkan::DeleteDevice(IRender::Device* device) {
	VulkanDeviceImpl* impl = static_cast<VulkanDeviceImpl*>(device);
	vkDestroySwapchainKHR(impl->device, impl->swapChain, allocator);
	vkDestroyDevice(impl->device, allocator);
	delete impl;
}

void ZRenderVulkan::SetDeviceResolution(IRender::Device* dev, const Int2& resolution) {
	VulkanDeviceImpl* impl = static_cast<VulkanDeviceImpl*>(dev);
	VkSwapchainKHR oldSwapChain = impl->swapChain;
	impl->swapChain = 0;

	// reset device swap chain
	VkDevice device = impl->device;
	Verify("wait idle", vkDeviceWaitIdle(device));

	VkSwapchainCreateInfoKHR info = {};
	info.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
	info.surface = surface;
	info.minImageCount = MIN_IMAGE_COUNT;
	info.imageFormat = VK_FORMAT_B8G8R8A8_UNORM;
	info.imageColorSpace = VK_COLORSPACE_SRGB_NONLINEAR_KHR;
	info.imageArrayLayers = 1;
	info.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
	info.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;           // Assume that graphics family == present family
	info.preTransform = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR;
	info.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
	info.presentMode = VK_PRESENT_MODE_FIFO_KHR;
	info.clipped = VK_TRUE;
	info.oldSwapchain = oldSwapChain;

	VkSurfaceCapabilitiesKHR cap;
	Verify("get physical device cap", vkGetPhysicalDeviceSurfaceCapabilitiesKHR(impl->physicalDevice, surface, &cap));
	if (info.minImageCount < cap.minImageCount)
		info.minImageCount = cap.minImageCount;
	else if (cap.maxImageCount != 0 && info.minImageCount > cap.maxImageCount)
		info.minImageCount = cap.maxImageCount;

	if (cap.currentExtent.width == 0xffffffff) {
		info.imageExtent.width = impl->resolution.x() = resolution.x();
		info.imageExtent.height = impl->resolution.y() = resolution.y();
	} else {
		info.imageExtent.width = impl->resolution.x() = cap.currentExtent.width;
		info.imageExtent.height = impl->resolution.y() = cap.currentExtent.height;
	}

	Verify("create swap chain", vkCreateSwapchainKHR(device, &info, allocator, &impl->swapChain));
	uint32_t imageCount;
	Verify("get swapchain images", vkGetSwapchainImagesKHR(device, impl->swapChain, &imageCount, NULL));

	impl->backBuffers.resize(imageCount);
	Verify("get swap chain images", vkGetSwapchainImagesKHR(device, impl->swapChain, &imageCount, &impl->backBuffers[0]));

	if (oldSwapChain)
		vkDestroySwapchainKHR(device, oldSwapChain, allocator);
}

Int2 ZRenderVulkan::GetDeviceResolution(IRender::Device* device) {
	VulkanDeviceImpl* impl = static_cast<VulkanDeviceImpl*>(device);
	return impl->resolution;
}

IRender::Queue* ZRenderVulkan::CreateQueue(Device* device, uint32_t flag) { return nullptr; }
IRender::Device* ZRenderVulkan::GetQueueDevice(Queue* queue) { return nullptr; }

void ZRenderVulkan::PresentQueues(Queue** queues, uint32_t count, PresentOption option) {}
bool ZRenderVulkan::SupportParallelPresent(Device* device) {
	return true;
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


ZRenderVulkan::ZRenderVulkan(GLFWwindow* win) : allocator(nullptr), window(win) {
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
	Verify("create vulkan instance (debug)", vkCreateInstance(&createInfo, allocator, &instance));

	// Get the function pointer (required for any extensions)
	auto vkCreateDebugReportCallbackEXT = (PFN_vkCreateDebugReportCallbackEXT)vkGetInstanceProcAddr(instance, "vkCreateDebugReportCallbackEXT");
	assert(vkCreateDebugReportCallbackEXT != NULL);

	// Setup the debug report callback
	VkDebugReportCallbackCreateInfoEXT debugReport = {};
	debugReport.sType = VK_STRUCTURE_TYPE_DEBUG_REPORT_CALLBACK_CREATE_INFO_EXT;
	debugReport.flags = VK_DEBUG_REPORT_ERROR_BIT_EXT | VK_DEBUG_REPORT_WARNING_BIT_EXT | VK_DEBUG_REPORT_PERFORMANCE_WARNING_BIT_EXT;
	debugReport.pfnCallback = DebugReportCallback;
	debugReport.pUserData = NULL;
	Verify("create debug report", vkCreateDebugReportCallbackEXT(instance, &debugReport, allocator, (VkDebugReportCallbackEXT*)&debugCallback));
#else
	// Create Vulkan Instance without any debug feature
	Verify("create vulkan instance (release)", vkCreateInstance(&createInfo, allocator, &instance));
#endif

	// Bind to window
	Verify("bind to window", glfwCreateWindowSurface(instance, window, allocator, (VkSurfaceKHR*)&surface));
}

ZRenderVulkan::~ZRenderVulkan() {
#ifdef _DEBUG
	PFN_vkDestroyDebugReportCallbackEXT vkDestroyDebugReportCallbackEXT = (PFN_vkDestroyDebugReportCallbackEXT)vkGetInstanceProcAddr(instance, "vkDestroyDebugReportCallbackEXT");
	vkDestroyDebugReportCallbackEXT(instance, (VkDebugReportCallbackEXT)debugCallback, allocator);
#endif

	vkDestroyInstance(instance, nullptr);
}
