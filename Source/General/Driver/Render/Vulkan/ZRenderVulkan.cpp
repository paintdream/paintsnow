#ifdef _MSC_VER
#pragma warning(disable:4786)
#endif

#include "../../../Interface/Interfaces.h"
#include "../../../Interface/IShader.h"
#include "../../../../Core/Interface/IMemory.h"
#include "../../../../Core/Template/TQueue.h"
#include "../../../../Core/Template/TPool.h"
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

struct FrameData {
	VkImage backBufferImage;
	VkImageView backBufferView;
	VkFence fence;
	VkSemaphore acquireSemaphore;
	VkSemaphore releaseSemaphore;
};

struct VulkanQueueImpl;
struct VulkanDeviceImpl : public IRender::Device {
	VulkanDeviceImpl(VkAllocationCallbacks* alloc, VkPhysicalDevice phy, VkDevice dev, uint32_t family, VkQueue q) : allocator(alloc), physicalDevice(phy), device(dev), resolution(0, 0), queueFamily(family), queue(q), swapChain(0) {
		critical.store(0, std::memory_order_release);
	}

	std::atomic<uint32_t> critical;
	VkPhysicalDevice physicalDevice;
	VkDevice device;
	VkAllocationCallbacks* allocator;
	VkQueue queue;
	Int2 resolution;
	uint32_t queueFamily;
	VkSwapchainKHR swapChain;

	std::vector<FrameData> frames;
	std::vector<VulkanQueueImpl*> deletedQueues;
};

struct VulkanQueueImpl : public IRender::Queue, public TPool<VulkanQueueImpl, VkCommandBuffer> {
	VulkanQueueImpl(VulkanDeviceImpl* dev, VkCommandPool pool) : device(dev), commandPool(pool), TPool<VulkanQueueImpl, VkCommandBuffer>(4), commandCount(0) {
		VkCommandBufferAllocateInfo info;
		info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
		info.commandBufferCount = maxCount;
		info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
		info.commandPool = commandPool;
		info.pNext = nullptr;

		// freeItems.reserve(maxCount);
		// vkAllocateCommandBuffers(device->device, &info, &freeItems[0]);
		currentCommandBuffer = Acquire();
	}

	~VulkanQueueImpl() {
		// delete all command buffers
		for (size_t k = 0; k < frames.size(); k++) {
			assert(frames[k].commandBuffers.empty());
		}

		if (!freeItems.empty()) {
			vkFreeCommandBuffers(device->device, commandPool, safe_cast<uint32_t>(freeItems.size()), &freeItems[0]);
			freeItems.clear();
		}

		vkFreeCommandBuffers(device->device, commandPool, 1, &currentCommandBuffer);
	}

	VkCommandBuffer New() {
		VkCommandBufferAllocateInfo info;
		info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
		info.commandBufferCount = 1;
		info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
		info.commandPool = commandPool;
		info.pNext = nullptr;

		VkCommandBuffer ret = nullptr;
		vkAllocateCommandBuffers(device->device, &info, &ret);
		return ret;
	}

	void Delete(VkCommandBuffer buffer) {
		vkFreeCommandBuffers(device->device, commandPool, 1, &buffer);
	}

	VulkanDeviceImpl* device;
	VkCommandPool commandPool;
	VkCommandBuffer currentCommandBuffer;
	uint32_t commandCount;

	struct Frame {
		std::vector<VkCommandBuffer> commandBuffers;
	};

	std::vector<Frame> frames;
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

			VulkanDeviceImpl* impl = new VulkanDeviceImpl(allocator, device, logicDevice, family, queue);

			int w, h;
			glfwGetFramebufferSize(window, &w, &h);
			SetDeviceResolution(impl, Int2(w, h));

			return impl;
		}
	}

	return nullptr;
}

static void CleanupFrameData(VkAllocationCallbacks* allocator, VulkanDeviceImpl* impl) {
	for (size_t i = 0; i < impl->frames.size(); i++) {
		FrameData& frameData = impl->frames[i];
		vkDestroySemaphore(impl->device, frameData.acquireSemaphore, allocator);
		vkDestroySemaphore(impl->device, frameData.releaseSemaphore, allocator);
		vkDestroyImage(impl->device, frameData.backBufferImage, allocator);
		vkDestroyImageView(impl->device, frameData.backBufferView, allocator);
		vkDestroyFence(impl->device, frameData.fence, allocator);
	}

	impl->frames.clear();
}


void ZRenderVulkan::DeleteDevice(IRender::Device* device) {
	VulkanDeviceImpl* impl = static_cast<VulkanDeviceImpl*>(device);
	CleanupFrameData(allocator, impl);
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
	info.surface = (VkSurfaceKHR)surface;
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
	Verify("get physical device cap", vkGetPhysicalDeviceSurfaceCapabilitiesKHR(impl->physicalDevice, (VkSurfaceKHR)surface, &cap));
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

	// Cleanup old frame data
	CleanupFrameData(allocator, impl);

	std::vector<VkImage> images(imageCount);
	Verify("get swap chain images", vkGetSwapchainImagesKHR(device, impl->swapChain, &imageCount, &images[0]));

	impl->frames.resize(imageCount);
	VkImageViewCreateInfo viewinfo = {};
	viewinfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
	viewinfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
	viewinfo.format = VK_FORMAT_B8G8R8A8_UNORM;
	viewinfo.components.r = VK_COMPONENT_SWIZZLE_R;
	viewinfo.components.g = VK_COMPONENT_SWIZZLE_G;
	viewinfo.components.b = VK_COMPONENT_SWIZZLE_B;
	viewinfo.components.a = VK_COMPONENT_SWIZZLE_A;
	VkImageSubresourceRange image_range = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 };
	viewinfo.subresourceRange = image_range;

	VkFenceCreateInfo fenceinfo = {};
	fenceinfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
	fenceinfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

	VkSemaphoreCreateInfo seminfo = {};
	seminfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

	for (uint32_t i = 0; i < imageCount; i++) {
		FrameData& frameData = impl->frames[i];
		viewinfo.image = frameData.backBufferImage = images[i];
		Verify("create image view", vkCreateImageView(device, &viewinfo, allocator, &frameData.backBufferView));
		Verify("create fence", vkCreateFence(device, &fenceinfo, allocator, &frameData.fence));
		Verify("create semaphore (acquire)", vkCreateSemaphore(device, &seminfo, allocator, &frameData.acquireSemaphore));
		Verify("create semaphore (release)", vkCreateSemaphore(device, &seminfo, allocator, &frameData.releaseSemaphore));
	}

	if (oldSwapChain)
		vkDestroySwapchainKHR(device, oldSwapChain, allocator);
}

Int2 ZRenderVulkan::GetDeviceResolution(IRender::Device* device) {
	VulkanDeviceImpl* impl = static_cast<VulkanDeviceImpl*>(device);
	return impl->resolution;
}

void ZRenderVulkan::NextDeviceFrame(IRender::Device* device) {
	// TODO:
}

IRender::Queue* ZRenderVulkan::CreateQueue(Device* dev, uint32_t flag) {
	VulkanDeviceImpl* device = static_cast<VulkanDeviceImpl*>(dev);
	VkCommandPoolCreateInfo info = {};
	info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
	info.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
	info.queueFamilyIndex = device->queueFamily;
	VkCommandPool commandPool;
	vkCreateCommandPool(device->device, &info, allocator, &commandPool);

	return new VulkanQueueImpl(device, commandPool);
}

IRender::Device* ZRenderVulkan::GetQueueDevice(Queue* q) {
	VulkanQueueImpl* queue = static_cast<VulkanQueueImpl*>(q);
	return queue->device;
}

bool ZRenderVulkan::SupportParallelPresent(Device* device) {
	return true;
}

void ZRenderVulkan::PresentQueues(Queue** queues, uint32_t count, PresentOption option) {}

bool ZRenderVulkan::IsQueueModified(Queue* q) {
	VulkanQueueImpl* queue = static_cast<VulkanQueueImpl*>(q);
	return queue->commandCount != 0;
}

void ZRenderVulkan::DeleteQueue(Queue* q) {
	VulkanQueueImpl* queue = static_cast<VulkanQueueImpl*>(q);
	SpinLock(queue->device->critical);
	queue->device->deletedQueues.emplace_back(queue); // delayed delete
	SpinUnLock(queue->device->critical);

	/*
	vkDestroyCommandPool(queue->device->device, queue->commandPool, allocator);
	delete queue;
	*/
}

void ZRenderVulkan::FlushQueue(Queue* q) {
	// start new segment
	VulkanQueueImpl* queue = static_cast<VulkanQueueImpl*>(q);
	uint32_t frame = frameIndex.load(std::memory_order_acquire);
	VulkanQueueImpl::Frame& currentFrame = queue->frames[frame % queue->frames.size()];
	currentFrame.commandBuffers.emplace_back(queue->currentCommandBuffer);
	queue->currentCommandBuffer = queue->AcquireSafe();
}

struct ResourceImplVulkanBase : public IRender::Resource {
	virtual void Upload(VulkanQueueImpl* queue, IRender::Resource::Description* description) = 0;
};

template <class T>
struct ResourceImplVulkanDesc : public ResourceImplVulkanBase {
	T description;
};

template <class T>
struct ResourceImplVulkan {};

template <>
struct ResourceImplVulkan<IRender::Resource::TextureDescription> : public ResourceImplVulkanDesc< IRender::Resource::TextureDescription> {
	ResourceImplVulkan() : image(0) {}

	virtual void Upload(VulkanQueueImpl* queue, IRender::Resource::Description* d) override {
		VulkanDeviceImpl* device = queue->device;
		IRender::Resource::TextureDescription& desc = *static_cast<IRender::Resource::TextureDescription*>(d);

		if (image == 0 || desc.dimension != description.dimension || desc.state != description.state) {
			if (image != 0) {
				vkDestroyImage(device->device, image, device->allocator);
			}

			VkImageCreateInfo info = {};
			info.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
			switch (desc.state.type) {
			case IRender::Resource::TextureDescription::TEXTURE_1D:
				info.imageType = VK_IMAGE_TYPE_1D;
				break;
			case IRender::Resource::TextureDescription::TEXTURE_2D:
				info.imageType = VK_IMAGE_TYPE_2D;
				break;
			case IRender::Resource::TextureDescription::TEXTURE_2D_CUBE:
				info.imageType = VK_IMAGE_TYPE_2D;
				break;
			case IRender::Resource::TextureDescription::TEXTURE_3D:
				info.imageType = VK_IMAGE_TYPE_3D;
				break;
			}

			info.extent.width = desc.dimension.x();
			info.extent.height = desc.dimension.y();
			info.extent.depth = desc.state.type == IRender::Resource::TextureDescription::TEXTURE_3D ? Math::Max((uint32_t)desc.dimension.z(), 1u) : 1u;
			info.mipLevels = desc.state.mip == IRender::Resource::TextureDescription::NOMIP ? 1 : Math::Log2((uint32_t)Math::Min(desc.dimension.x(), desc.dimension.y())) + 1;
			info.arrayLayers = desc.state.type != IRender::Resource::TextureDescription::TEXTURE_3D ? Math::Max((uint32_t)desc.dimension.z(), 1u) : 1u;
			info.samples = VK_SAMPLE_COUNT_1_BIT;
			info.tiling = VK_IMAGE_TILING_OPTIMAL;
			info.usage = desc.state.attachment ? VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT: VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
			info.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;

			Verify("create image", vkCreateImage(device->device, &info, device->allocator, &image));

			VkMemoryRequirements req;
			vkGetImageMemoryRequirements(device->device, image, &req);

			VkMemoryAllocateInfo allocInfo;
			allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
			allocInfo.allocationSize = req.size;
			allocInfo.memoryTypeIndex = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;

			VkDeviceMemory memory;
			Verify("allocate texture memory", vkAllocateMemory(device->device, &allocInfo, device->allocator, &memory));
			Verify("bind image", vkBindImageMemory(device->device, image, memory, 0));
		}

		// TODO: update data

		description = std::move(desc);
	}

	VkImage image;
};

template <>
struct ResourceImplVulkan<IRender::Resource::BufferDescription> : public ResourceImplVulkanBase {
	virtual void Upload(VulkanQueueImpl* queue, IRender::Resource::Description* description) override {

	}
};

template <>
struct ResourceImplVulkan<IRender::Resource::ShaderDescription> : public ResourceImplVulkanBase {
	virtual void Upload(VulkanQueueImpl* queue, IRender::Resource::Description* description) override {

	}
};

template <>
struct ResourceImplVulkan<IRender::Resource::RenderStateDescription> : public ResourceImplVulkanBase {
	virtual void Upload(VulkanQueueImpl* queue, IRender::Resource::Description* description) override {

	}
};

template <>
struct ResourceImplVulkan<IRender::Resource::RenderTargetDescription> : public ResourceImplVulkanBase {
	virtual void Upload(VulkanQueueImpl* queue, IRender::Resource::Description* description) override {

	}
};

template <>
struct ResourceImplVulkan<IRender::Resource::ClearDescription> : public ResourceImplVulkanBase {
	virtual void Upload(VulkanQueueImpl* queue, IRender::Resource::Description* description) override {

	}
};

template <>
struct ResourceImplVulkan<IRender::Resource::DrawCallDescription> : public ResourceImplVulkanBase {
	virtual void Upload(VulkanQueueImpl* queue, IRender::Resource::Description* description) override {

	}
};


// Resource
IRender::Resource* ZRenderVulkan::CreateResource(Device* device, Resource::Type resourceType) {
	switch (resourceType)
	{
	case Resource::RESOURCE_TEXTURE:
		return new ResourceImplVulkan<Resource::TextureDescription>();
	case Resource::RESOURCE_BUFFER:
		return new ResourceImplVulkan<Resource::BufferDescription>();
	case Resource::RESOURCE_SHADER:
		return new ResourceImplVulkan<Resource::ShaderDescription>();
	case Resource::RESOURCE_RENDERSTATE:
		return new ResourceImplVulkan<Resource::RenderStateDescription>();
	case Resource::RESOURCE_RENDERTARGET:
		return new ResourceImplVulkan<Resource::RenderTargetDescription>();
	case Resource::RESOURCE_CLEAR:
		return new ResourceImplVulkan<Resource::ClearDescription>();
	case Resource::RESOURCE_DRAWCALL:
		return new ResourceImplVulkan<Resource::DrawCallDescription>();
	}

	assert(false);
	return nullptr;
}

void ZRenderVulkan::UploadResource(Queue* queue, Resource* resource, Resource::Description* description) {
	ResourceImplVulkanBase* res = static_cast<ResourceImplVulkanBase*>(resource);
	res->Upload(static_cast<VulkanQueueImpl*>(queue), description);
}

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
	frameIndex.store(0, std::memory_order_release);
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
