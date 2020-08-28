#ifdef _MSC_VER
#pragma warning(disable:4786)
#endif

#include "../../../Interface/Interfaces.h"
#include "../../../Interface/IShader.h"
#include "../../../../Core/Interface/IMemory.h"
#include "../../../../Core/Template/TQueue.h"
#include "../../../../Core/Template/TPool.h"
#include "../OpenGL/GLSLShaderGenerator.h"
#include "ZRenderVulkan.h"
// #include <glslang/Public/ShaderLang.h>
#include <cstdio>
#include <vector>
#include <iterator>
#include <sstream>

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

using namespace PaintsNow;

const int MIN_IMAGE_COUNT = 2;
const int MAX_DESCRIPTOR_SIZE = 1000;

static void Verify(const char* message, VkResult res) {
	if (res != VK_SUCCESS) {
		fprintf(stderr, "Unable to %s (debug).\n", message);
		exit(0);
	}
}

struct VulkanQueueImpl;
struct ResourceImplVulkanBase : public IRender::Resource {
	virtual void Upload(VulkanQueueImpl* queue, IRender::Resource::Description* description) = 0;
	virtual void Delete(VulkanQueueImpl* queue) = 0;
	virtual void Execute(VulkanQueueImpl* queue) = 0;
};

template <class T>
struct ResourceImplVulkanDesc : public ResourceImplVulkanBase {
	typedef ResourceImplVulkanDesc BaseClass;
	virtual void Upload(VulkanQueueImpl* queue, IRender::Resource::Description* description) override {
		cacheDescription = std::move(*static_cast<T*>(description));
	}

	virtual void Delete(VulkanQueueImpl* queue) override {}
	virtual void Execute(VulkanQueueImpl* queue) override {}

	T cacheDescription;
};

struct FrameData {
	VkImage backBufferImage;
	VkImageView backBufferView;
	VkFence fence;
	VkSemaphore acquireSemaphore;
	VkSemaphore releaseSemaphore;
};

struct VulkanQueueImpl;
struct VulkanDeviceImpl : public IRender::Device {
	VulkanDeviceImpl(VkAllocationCallbacks* alloc, VkPhysicalDevice phy, VkDevice dev, uint32_t family, VkQueue q, VkDescriptorPool pool) : allocator(alloc), physicalDevice(phy), device(dev), resolution(0, 0), queueFamily(family), queue(q), swapChain(VK_NULL_HANDLE), descriptorPool(pool) {
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
	VkDescriptorPool descriptorPool;

	std::vector<FrameData> frames;
	std::vector<VulkanQueueImpl*> deletedQueues;
};

struct VulkanQueueImpl : public IRender::Queue, public TPool<VulkanQueueImpl, VkCommandBuffer> {
	VulkanQueueImpl(VulkanDeviceImpl* dev, VkCommandPool pool) : device(dev), commandPool(pool), TPool<VulkanQueueImpl, VkCommandBuffer>(4), commandCount(0), renderTargetResource(nullptr) {
		VkCommandBufferAllocateInfo info;
		info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
		info.commandBufferCount = maxCount;
		info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
		info.commandPool = commandPool;
		info.pNext = nullptr;

		currentCommandBuffer = Acquire();
	}

	~VulkanQueueImpl() {
		while (!deletedResources.Empty()) {
			ResourceImplVulkanBase* res = deletedResources.Top();
			res->Delete(this);
			delete res;

			deletedResources.Pop();
		}

		freeItems.clear(); // VkCommandBuffer can be automatically freed as command pool destroys
		vkDestroyCommandPool(device->device, commandPool, device->allocator);
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

	void FlushCommandBuffer() {
		preparedCommandBuffers.Push(currentCommandBuffer);
		commandCount = 0;
		currentCommandBuffer = Acquire();
		vkResetCommandBuffer(currentCommandBuffer, VK_COMMAND_BUFFER_RESET_RELEASE_RESOURCES_BIT);
	}

	void BeginFrame() {
		preparedCommandBuffers.Push((VkCommandBuffer)VK_NULL_HANDLE);
		transientDataBuffers.Push((VkBuffer)VK_NULL_HANDLE);
	}

	void EndFrame() {
		for (VkCommandBuffer commandBuffer = preparedCommandBuffers.Top(); commandBuffer != VK_NULL_HANDLE; commandBuffer = preparedCommandBuffers.Top()) {
			Release(commandBuffer);
			preparedCommandBuffers.Pop();
		}

		for (VkBuffer buffer = transientDataBuffers.Top(); buffer != VK_NULL_HANDLE; buffer = transientDataBuffers.Top()) {
			vkDestroyBuffer(device->device, buffer, device->allocator);
			transientDataBuffers.Pop();
		}
	}

	VulkanDeviceImpl* device;
	VkCommandPool commandPool;
	VkCommandBuffer currentCommandBuffer;
	uint32_t commandCount;

	IRender::Resource::RenderStateDescription renderStateDescription;
	ResourceImplVulkanBase* renderTargetResource;
	TQueueList<VkBuffer, 4> transientDataBuffers;
	TQueueList<VkCommandBuffer, 4> preparedCommandBuffers;
	TQueueList<ResourceImplVulkanBase*> deletedResources;
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

			const char* deviceExtensions[] = { "VK_KHR_swapchain", "VK_NV_glsl_shader" };
			int deviceExtensionCount = sizeof(deviceExtensions) / sizeof(deviceExtensions[0]);
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

			VkDescriptorPoolSize poolSize[] =
			{
				{ VK_DESCRIPTOR_TYPE_SAMPLER, MAX_DESCRIPTOR_SIZE },
				{ VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, MAX_DESCRIPTOR_SIZE },
				{ VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, MAX_DESCRIPTOR_SIZE },
				{ VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, MAX_DESCRIPTOR_SIZE },
				{ VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER, MAX_DESCRIPTOR_SIZE },
				{ VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER, MAX_DESCRIPTOR_SIZE },
				{ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, MAX_DESCRIPTOR_SIZE },
				{ VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, MAX_DESCRIPTOR_SIZE },
				{ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, MAX_DESCRIPTOR_SIZE },
				{ VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC, MAX_DESCRIPTOR_SIZE },
				{ VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, MAX_DESCRIPTOR_SIZE }
			};

			VkDescriptorPool descriptorPool;
			VkDescriptorPoolCreateInfo poolInfo = {};
			poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
			poolInfo.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
			poolInfo.maxSets = MAX_DESCRIPTOR_SIZE * sizeof(poolSize);
			poolInfo.poolSizeCount = (uint32_t)sizeof(poolSize);
			poolInfo.pPoolSizes = poolSize;
			Verify("create descriptor pool", vkCreateDescriptorPool(logicDevice, &poolInfo, allocator, &descriptorPool));

			VulkanDeviceImpl* impl = new VulkanDeviceImpl(allocator, device, logicDevice, family, queue, descriptorPool);

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
	vkDestroyDescriptorPool(impl->device, impl->descriptorPool, allocator);
	vkDestroyDevice(impl->device, allocator);

	delete impl;
}

void ZRenderVulkan::SetDeviceResolution(IRender::Device* dev, const Int2& resolution) {
	VulkanDeviceImpl* impl = static_cast<VulkanDeviceImpl*>(dev);
	VkSwapchainKHR oldSwapChain = impl->swapChain;
	impl->swapChain = VK_NULL_HANDLE;

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
	queue->FlushCommandBuffer();
}

template <class T>
struct ResourceImplVulkan {};

static VkFormat TranslateFormat(uint32_t format, uint32_t layout) {
	static const VkFormat formats[IRender::Resource::TextureDescription::Layout::END][IRender::Resource::Description::Format::END] = {
		{ VK_FORMAT_R8_UNORM, VK_FORMAT_R16_UNORM, VK_FORMAT_R16_SFLOAT, VK_FORMAT_R32_SFLOAT, VK_FORMAT_R32_UINT },
		{ VK_FORMAT_R8G8_UNORM, VK_FORMAT_R16G16_UNORM, VK_FORMAT_R16G16_SFLOAT, VK_FORMAT_R32G32_SFLOAT, VK_FORMAT_R32G32_UINT },
		{ VK_FORMAT_R8G8B8_UNORM, VK_FORMAT_R16G16B16_UNORM, VK_FORMAT_R16G16B16_SFLOAT, VK_FORMAT_R32G32B32_SFLOAT, VK_FORMAT_R32G32B32_UINT },
		{ VK_FORMAT_R8G8B8A8_UNORM, VK_FORMAT_R16G16B16A16_UNORM, VK_FORMAT_R16G16B16A16_SFLOAT, VK_FORMAT_R32G32B32A32_SFLOAT, VK_FORMAT_R32G32B32A32_UINT },
		{ VK_FORMAT_UNDEFINED, VK_FORMAT_D16_UNORM, VK_FORMAT_D16_UNORM, VK_FORMAT_D32_SFLOAT, VK_FORMAT_UNDEFINED },
		{ VK_FORMAT_S8_UINT, VK_FORMAT_UNDEFINED, VK_FORMAT_UNDEFINED, VK_FORMAT_UNDEFINED, VK_FORMAT_UNDEFINED },
		{ VK_FORMAT_UNDEFINED, VK_FORMAT_UNDEFINED, VK_FORMAT_UNDEFINED, VK_FORMAT_D24_UNORM_S8_UINT, VK_FORMAT_UNDEFINED },
		{ VK_FORMAT_A2R10G10B10_UNORM_PACK32, VK_FORMAT_A2R10G10B10_UNORM_PACK32, VK_FORMAT_B10G11R11_UFLOAT_PACK32, VK_FORMAT_UNDEFINED, VK_FORMAT_UNDEFINED },
	};

	return formats[layout][format];
}

template <>
struct ResourceImplVulkan<IRender::Resource::TextureDescription> : public ResourceImplVulkanDesc<IRender::Resource::TextureDescription> {
	ResourceImplVulkan() : image(VK_NULL_HANDLE), imageView(VK_NULL_HANDLE) {}
	~ResourceImplVulkan() {
	}

	void Clear(VulkanDeviceImpl* device) {
		if (imageView != VK_NULL_HANDLE) {
			vkDestroyImageView(device->device, imageView, device->allocator);
			imageView = VK_NULL_HANDLE;
		}

		if (image != VK_NULL_HANDLE) {
			vkDestroyImage(device->device, image, device->allocator);
			image = VK_NULL_HANDLE;
		}
	}

	virtual void Delete(VulkanQueueImpl* queue) override {
		Clear(queue->device);
	}

	virtual void Upload(VulkanQueueImpl* queue, IRender::Resource::Description* d) override {
		VulkanDeviceImpl* device = queue->device;
		IRender::Resource::TextureDescription& desc = *static_cast<IRender::Resource::TextureDescription*>(d);

		if (image == 0 || cacheDescription.dimension != desc.dimension || cacheDescription.state != desc.state) {
			Clear(device);

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

			info.format = TranslateFormat(desc.state.format, desc.state.layout);
			info.extent.width = desc.dimension.x();
			info.extent.height = desc.dimension.y();
			info.extent.depth = desc.state.type == IRender::Resource::TextureDescription::TEXTURE_3D ? Math::Max((uint32_t)desc.dimension.z(), 1u) : 1u;
			info.mipLevels = desc.state.mip == IRender::Resource::TextureDescription::NOMIP ? 1 : Math::Log2((uint32_t)Math::Min(desc.dimension.x(), desc.dimension.y())) + 1;
			info.arrayLayers = desc.state.type != IRender::Resource::TextureDescription::TEXTURE_3D ? Math::Max((uint32_t)desc.dimension.z(), 1u) : 1u;
			info.samples = VK_SAMPLE_COUNT_1_BIT;
			info.tiling = VK_IMAGE_TILING_OPTIMAL;
			info.usage = desc.state.attachment ? VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT : VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
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

			VkImageViewCreateInfo viewInfo = {};
			viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
			viewInfo.image = image;
			viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
			viewInfo.format = VK_FORMAT_R8G8B8A8_UNORM;
			viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			viewInfo.subresourceRange.levelCount = 1;
			viewInfo.subresourceRange.layerCount = 1;
			Verify("create image view", vkCreateImageView(device->device, &viewInfo, device->allocator, &imageView));
		}

		if (!desc.data.Empty()) {
			VkBufferCreateInfo bufferInfo = {};
			bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
			bufferInfo.size = desc.data.GetSize();
			bufferInfo.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
			bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
			VkBuffer uploadBuffer;
			Verify("create buffer", vkCreateBuffer(device->device, &bufferInfo, device->allocator, &uploadBuffer));

			VkMemoryRequirements req;
			vkGetBufferMemoryRequirements(device->device, uploadBuffer, &req);
			// assert(((size_t)description.data.GetData() & ~req.alignment) == 0);
			VkMemoryAllocateInfo allocInfo = {};
			allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
			allocInfo.allocationSize = req.size;
			allocInfo.memoryTypeIndex = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT;

			VkDeviceMemory deviceMemory;
			Verify("allocate memory", vkAllocateMemory(device->device, &allocInfo, device->allocator, &deviceMemory));
			Verify("bind memory", vkBindBufferMemory(device->device, uploadBuffer, deviceMemory, 0));

			void* map = nullptr;
			Verify("map memory", vkMapMemory(device->device, deviceMemory, 0, desc.data.GetSize(), 0, &map));
			memcpy(map, desc.data.GetData(), desc.data.GetSize());
			VkMappedMemoryRange range = {};
			range.sType = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE;
			range.memory = deviceMemory;
			range.size = desc.data.GetSize();
			Verify("flush memory", vkFlushMappedMemoryRanges(device->device, 1, &range));
			vkUnmapMemory(device->device, deviceMemory);

			// Copy buffer to image
			VkImageMemoryBarrier copyBarrier = {};
			copyBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
			copyBarrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
			copyBarrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
			copyBarrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
			copyBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
			copyBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
			copyBarrier.image = image;
			copyBarrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			copyBarrier.subresourceRange.levelCount = 1;
			copyBarrier.subresourceRange.layerCount = 1;
			vkCmdPipelineBarrier(queue->currentCommandBuffer, VK_PIPELINE_STAGE_HOST_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, NULL, 0, NULL, 1, &copyBarrier);

			VkBufferImageCopy region = {};
			region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			region.imageSubresource.layerCount = 1;
			region.imageExtent.width = desc.dimension.x();
			region.imageExtent.height = desc.dimension.y();
			region.imageExtent.depth = desc.state.type == IRender::Resource::TextureDescription::TEXTURE_3D ? Math::Max((uint32_t)desc.dimension.z(), 1u) : 1u;
			vkCmdCopyBufferToImage(queue->currentCommandBuffer, uploadBuffer, image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);

			VkImageMemoryBarrier useBarrier = {};
			useBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
			useBarrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
			useBarrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
			useBarrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
			useBarrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
			useBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
			useBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
			useBarrier.image = image;
			useBarrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			useBarrier.subresourceRange.levelCount = 1;
			useBarrier.subresourceRange.layerCount = 1;
			vkCmdPipelineBarrier(queue->currentCommandBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0, 0, NULL, 0, NULL, 1, &useBarrier);

			queue->transientDataBuffers.Push(uploadBuffer);
			desc.data.Clear();
		}
	
		BaseClass::Upload(queue, d);
	}

	VkImage image;
	VkImageView imageView;
};

template <>
struct ResourceImplVulkan<IRender::Resource::BufferDescription> : public ResourceImplVulkanDesc<IRender::Resource::BufferDescription> {
	ResourceImplVulkan() : buffer(VK_NULL_HANDLE) {}
	~ResourceImplVulkan() {
		assert(buffer == VK_NULL_HANDLE);
	}

	void Clear(VulkanDeviceImpl* device) {
		if (buffer != VK_NULL_HANDLE) {
			vkDestroyBuffer(device->device, buffer, device->allocator);
			buffer = VK_NULL_HANDLE;
		}
	}

	virtual void Delete(VulkanQueueImpl* queue) override {
		Clear(queue->device);
	}

	virtual void Upload(VulkanQueueImpl* queue, IRender::Resource::Description* d) override {
		VulkanDeviceImpl* device = queue->device;
		if (buffer != VK_NULL_HANDLE) {
			Clear(device);
		}

		VkBufferCreateInfo createInfo = {};
		createInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
		IRender::Resource::BufferDescription& desc = *static_cast<IRender::Resource::BufferDescription*>(d);
		createInfo.size = desc.data.GetSize();
		switch (desc.usage) {
		case IRender::Resource::BufferDescription::INDEX:
			createInfo.usage = VK_BUFFER_USAGE_INDEX_BUFFER_BIT;
			break;
		case IRender::Resource::BufferDescription::VERTEX:
			createInfo.usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
			break;
		case IRender::Resource::BufferDescription::INSTANCED:
			createInfo.usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
			break;
		case IRender::Resource::BufferDescription::UNIFORM:
			createInfo.usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
			break;
		case IRender::Resource::BufferDescription::STORAGE:
			createInfo.usage = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;
			break;
		}

		createInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
		Verify("create buffer", vkCreateBuffer(device->device, &createInfo, device->allocator, &buffer));

		VkMemoryRequirements req;
		vkGetBufferMemoryRequirements(device->device, buffer, &req);
		VkMemoryAllocateInfo allocInfo = {};
		allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
		allocInfo.allocationSize = req.size;
		allocInfo.memoryTypeIndex = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT;

		VkDeviceMemory deviceMemory;
		Verify("allocate memory", vkAllocateMemory(device->device, &allocInfo, device->allocator, &deviceMemory));
		Verify("bind memory", vkBindBufferMemory(device->device, buffer, deviceMemory, 0));

		void* map = nullptr;
		Verify("map memory", vkMapMemory(device->device, deviceMemory, 0, desc.data.GetSize(), 0, &map));
		memcpy(map, desc.data.GetData(), desc.data.GetSize());
		VkMappedMemoryRange range = {};
		range.sType = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE;
		range.memory = deviceMemory;
		range.size = desc.data.GetSize();
		Verify("flush memory", vkFlushMappedMemoryRanges(device->device, 1, &range));
		vkUnmapMemory(device->device, deviceMemory);

		desc.data.Clear();
		BaseClass::Upload(queue, d);
	}

	VkBuffer buffer;
};

template <>
struct ResourceImplVulkan<IRender::Resource::DrawCallDescription> : public ResourceImplVulkanDesc<IRender::Resource::DrawCallDescription> {
	uint32_t GetVertexBufferCount() {
		return signature.GetSize() / 3;
	}

	uint8_t GetVertexBufferBindingIndex(uint32_t index) {
		return signature.GetData()[index * 3];
	}

	virtual void Upload(VulkanQueueImpl* queue, IRender::Resource::Description* d) override {
		// Compute signature
		IRender::Resource::DrawCallDescription* description = static_cast<IRender::Resource::DrawCallDescription*>(d);
		signature.Resize(description->bufferResources.size() * 3);
		uint8_t* data = signature.GetData();

		for (size_t i = 0; i < description->bufferResources.size(); i++) {
			const IRender::Resource::DrawCallDescription::BufferRange& bufferRange = description->bufferResources[i];
			assert(bufferRange.buffer != nullptr);

			ResourceImplVulkan<IRender::Resource::BufferDescription>* buffer = static_cast<ResourceImplVulkan<IRender::Resource::BufferDescription>*>(bufferRange.buffer);
			IRender::Resource::BufferDescription& desc = buffer->cacheDescription;
			if (desc.usage == IRender::Resource::BufferDescription::VERTEX || desc.usage == IRender::Resource::BufferDescription::INSTANCED) {
				assert(desc.component < 32);
				assert(bufferRange.offset <= 255);

				uint8_t sameIndex = safe_cast<uint8_t>(i);
				for (size_t n = 0; n < i; n++) {
					if (description->bufferResources[n].buffer == buffer) {
						sameIndex = safe_cast<uint8_t>(n);
						break;
					}
				}

				*data++ = sameIndex;
				*data++ = desc.format << 5 | desc.component;
				*data++ = safe_cast<uint8_t>(bufferRange.offset);
			}
		}

		BaseClass::Upload(queue, description);
	}

	Bytes signature;
};

struct PipelineInstance {
	VkPipeline pipeline;
};

static VkCompareOp ConvertCompareOperation(uint32_t op) {
	switch (op) {
	case IRender::Resource::RenderStateDescription::DISABLED:
		return VK_COMPARE_OP_NEVER;
	case IRender::Resource::RenderStateDescription::ALWAYS:
		return VK_COMPARE_OP_ALWAYS;
	case IRender::Resource::RenderStateDescription::LESS:
		return VK_COMPARE_OP_LESS;
	case IRender::Resource::RenderStateDescription::LESS_EQUAL:
		return VK_COMPARE_OP_LESS_OR_EQUAL;
	case IRender::Resource::RenderStateDescription::GREATER:
		return VK_COMPARE_OP_GREATER;
	case IRender::Resource::RenderStateDescription::GREATER_EQUAL:
		return VK_COMPARE_OP_GREATER_OR_EQUAL;
	case IRender::Resource::RenderStateDescription::EQUAL:
		return VK_COMPARE_OP_EQUAL;
	}

	return VK_COMPARE_OP_ALWAYS;
}

struct PipelineKey {
	bool operator < (const PipelineKey& rhs) const {
		int n = memcmp(&renderState, &rhs.renderState, sizeof(renderState));
		if (n != 0) {
			return n < 0;
		} else if (renderTargetSignature < rhs.renderTargetSignature) {
			return true;
		} else if (renderTargetSignature > rhs.renderTargetSignature) {
			return false;
		} else {
			return bufferSignature < rhs.bufferSignature;
		}
	}

	IRender::Resource::RenderStateDescription renderState;
	uint32_t renderTargetSignature;
	Bytes bufferSignature;
};

static uint32_t ComputeBufferStride(const IRender::Resource::BufferDescription& desc) {
	uint32_t unit = 1;
	switch (desc.format) {
	case IRender::Resource::Description::UNSIGNED_BYTE:
		unit = 1;
		break;
	case IRender::Resource::Description::UNSIGNED_SHORT:
	case IRender::Resource::Description::HALF:
		unit = 2;
		break;
	case IRender::Resource::Description::FLOAT:
		unit = 3;
		break;
	}

	return unit * desc.component;
}


template <>
struct ResourceImplVulkan<IRender::Resource::RenderStateDescription> : public ResourceImplVulkanDesc<IRender::Resource::RenderStateDescription> {
	virtual void Execute(VulkanQueueImpl* queue) override {
		queue->renderStateDescription = cacheDescription;
	}
};

static uint32_t EncodeRenderTargetSignature(const IRender::Resource::RenderTargetDescription& desc) {
	uint32_t sig = 0;
	sig = (sig << 3) | (desc.depthStorage.loadOp << 1) | (desc.depthStorage.storeOp);
	sig = (sig << 3) | (desc.stencilStorage.loadOp << 1) | (desc.stencilStorage.storeOp);

	for (size_t i = 0; i < desc.colorStorages.size(); i++) {
		const IRender::Resource::RenderTargetDescription::Storage& s = desc.colorStorages[i];
		assert(s.loadOp < 4 && s.storeOp < 2);
		sig = (sig << 3) | (s.loadOp << 1) | (s.storeOp);
	}

	return sig;
}

template <>
struct ResourceImplVulkan<IRender::Resource::RenderTargetDescription> : public ResourceImplVulkanDesc<IRender::Resource::RenderTargetDescription> {
	ResourceImplVulkan() : signature(0), renderPass(VK_NULL_HANDLE), frameBuffer(VK_NULL_HANDLE) {}

	static VkAttachmentLoadOp ConvertLoadOp(uint32_t k) {
		switch (k) {
		case IRender::Resource::RenderTargetDescription::DEFAULT:
			return VK_ATTACHMENT_LOAD_OP_LOAD;
		case IRender::Resource::RenderTargetDescription::DISCARD:
			return VK_ATTACHMENT_LOAD_OP_DONT_CARE;
		case IRender::Resource::RenderTargetDescription::CLEAR:
			return VK_ATTACHMENT_LOAD_OP_CLEAR;
		}

		assert(false);
		return VK_ATTACHMENT_LOAD_OP_LOAD;
	}

	static VkAttachmentStoreOp ConvertStoreOp(uint32_t k) {
		switch (k) {
		case IRender::Resource::RenderTargetDescription::DEFAULT:
			return VK_ATTACHMENT_STORE_OP_STORE;
		case IRender::Resource::RenderTargetDescription::DISCARD:
			return VK_ATTACHMENT_STORE_OP_DONT_CARE;
		}

		assert(false);
		return VK_ATTACHMENT_STORE_OP_STORE;
	}

	virtual void Delete(VulkanQueueImpl* queue) {
		VulkanDeviceImpl* device = queue->device;
		if (renderPass != VK_NULL_HANDLE) {
			vkDestroyRenderPass(device->device, renderPass, device->allocator);
			renderPass = VK_NULL_HANDLE;
		}

		if (frameBuffer != VK_NULL_HANDLE) {
			vkDestroyFramebuffer(device->device, frameBuffer, device->allocator);
			frameBuffer = VK_NULL_HANDLE;
		}
	}

	virtual void Upload(VulkanQueueImpl* queue, IRender::Resource::Description* desc) override {
		BaseClass::Upload(queue, desc);
		signature = EncodeRenderTargetSignature(cacheDescription);

		std::vector<VkAttachmentDescription> attachmentDescriptions(cacheDescription.colorStorages.size());
		std::vector<VkAttachmentReference> attachmentReferences(attachmentDescriptions.size());
		for (size_t i = 0; i < cacheDescription.colorStorages.size(); i++) {
			const IRender::Resource::RenderTargetDescription::Storage& storage = cacheDescription.colorStorages[i];
			ResourceImplVulkan<IRender::Resource::TextureDescription>* resource = static_cast<ResourceImplVulkan<IRender::Resource::TextureDescription>*>(storage.resource);

			VkAttachmentDescription& desc = attachmentDescriptions[i];
			desc.format = TranslateFormat(resource->cacheDescription.state.format, resource->cacheDescription.state.layout);
			desc.samples = VK_SAMPLE_COUNT_1_BIT;
			desc.loadOp = ConvertLoadOp(desc.loadOp);
			desc.storeOp = ConvertStoreOp(desc.storeOp);
			desc.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
			desc.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
			desc.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
			desc.finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

			VkAttachmentReference& colorAttachment = attachmentReferences[i];
			colorAttachment.attachment = 0;
			colorAttachment.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
		}

		VkAttachmentReference depthStencilAttachment;
		depthStencilAttachment.attachment = 0;
		depthStencilAttachment.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

		VkSubpassDescription subPass = {};
		subPass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
		subPass.colorAttachmentCount = attachmentReferences.size();
		subPass.pColorAttachments = &attachmentReferences[0];
		subPass.pDepthStencilAttachment = &depthStencilAttachment;

		VkSubpassDependency dependency = {};
		dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
		dependency.dstSubpass = 0;
		dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
		dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
		dependency.srcAccessMask = 0;
		dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

		VkRenderPassCreateInfo renderPassInfo = {};
		renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
		renderPassInfo.attachmentCount = attachmentDescriptions.size();
		renderPassInfo.pAttachments = &attachmentDescriptions[0];
		renderPassInfo.subpassCount = 1;
		renderPassInfo.pSubpasses = &subPass;
		renderPassInfo.dependencyCount = 1;
		renderPassInfo.pDependencies = &dependency;

		VkRenderPass renderPass;
		VulkanDeviceImpl* device = queue->device;
		Verify("create render pass", vkCreateRenderPass(device->device, &renderPassInfo, device->allocator, &renderPass));

		std::vector<VkImageView> attachments(cacheDescription.colorStorages.size());
		for (size_t i = 0; i < attachments.size(); i++) {
			ResourceImplVulkan<IRender::Resource::TextureDescription>* texture = static_cast<ResourceImplVulkan<IRender::Resource::TextureDescription>*>(cacheDescription.colorStorages[i].resource);
			attachments[i] = texture->imageView;
		}

		VkFramebufferCreateInfo info = {};
		info.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
		info.renderPass = renderPass;
		info.attachmentCount = cacheDescription.colorStorages.size();
	}

	virtual void Execute(VulkanQueueImpl* queue) override {
		queue->renderTargetResource = this;

		// std::vector<VkImageView> attachments()
	}

	uint32_t signature;
	VkRenderPass renderPass;
	VkFramebuffer frameBuffer;
};

template <>
struct ResourceImplVulkan<IRender::Resource::ShaderDescription> : public ResourceImplVulkanBase {
	ResourceImplVulkan() : descriptorSetLayout(VK_NULL_HANDLE), descriptorSet(VK_NULL_HANDLE), pipelineLayout(VK_NULL_HANDLE) {
		memset(shaderModules, 0, sizeof(shaderModules));
	}

	void Clear(VulkanDeviceImpl* device) {
		if (pipelineLayout != VK_NULL_HANDLE) {
			vkDestroyPipelineLayout(device->device, pipelineLayout, device->allocator);
			pipelineLayout = VK_NULL_HANDLE;
		}

		if (descriptorSetLayout != VK_NULL_HANDLE) {
			vkDestroyDescriptorSetLayout(device->device, descriptorSetLayout, device->allocator);
			descriptorSetLayout = VK_NULL_HANDLE;
		}

		if (descriptorSet != VK_NULL_HANDLE) {
			vkFreeDescriptorSets(device->device, device->descriptorPool, 1, &descriptorSet);
			descriptorSet = VK_NULL_HANDLE;
		}

		for (size_t i = 0; i < sizeof(shaderModules) / sizeof(shaderModules[0]); i++) {
			VkShaderModule& m = shaderModules[i];
			if (m != VK_NULL_HANDLE) {
				vkDestroyShaderModule(device->device, m, device->allocator);
				m = VK_NULL_HANDLE;
			}
		}
	}


	PipelineInstance& QueryInstance(VulkanQueueImpl* queue, ResourceImplVulkan<IRender::Resource::DrawCallDescription>* drawCall) {
		// Generate vertex format.
		const IRender::Resource::RenderStateDescription& renderState = queue->renderStateDescription;
		PipelineKey key;
		key.renderState = renderState;
		key.bufferSignature = drawCall->signature;
		assert(queue->renderTargetResource != nullptr);
		ResourceImplVulkan<IRender::Resource::RenderTargetDescription>* target = static_cast<ResourceImplVulkan<IRender::Resource::RenderTargetDescription>*>(queue->renderTargetResource);
		key.renderTargetSignature = target->signature;

		std::vector<std::key_value<PipelineKey, PipelineInstance> >::iterator it = std::binary_find(stateInstances.begin(), stateInstances.end(), key);
		if (it != stateInstances.end()) {
			return it->second;
		}

		VkPipelineDepthStencilStateCreateInfo depthInfo = {};
		depthInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
		depthInfo.depthTestEnable = renderState.depthTest != IRender::Resource::RenderStateDescription::DISABLED;
		depthInfo.depthWriteEnable = renderState.depthWrite;
		depthInfo.depthCompareOp = ConvertCompareOperation(renderState.depthTest);
		depthInfo.depthBoundsTestEnable = false;
		depthInfo.stencilTestEnable = renderState.stencilTest != IRender::Resource::RenderStateDescription::DISABLED;
		depthInfo.front.compareOp = ConvertCompareOperation(renderState.stencilTest);
		depthInfo.front.compareMask = renderState.stencilMask;
		depthInfo.front.depthFailOp = renderState.stencilReplaceZFail ? VK_STENCIL_OP_REPLACE : VK_STENCIL_OP_KEEP;
		depthInfo.front.failOp = renderState.stencilReplaceFail ? VK_STENCIL_OP_REPLACE : VK_STENCIL_OP_KEEP;
		depthInfo.front.passOp = renderState.stencilReplacePass ? VK_STENCIL_OP_REPLACE : VK_STENCIL_OP_KEEP;
		depthInfo.front.reference = renderState.stencilValue;
		depthInfo.front.writeMask = renderState.stencilWrite ? 0xFFFFFFFF : 0;
		depthInfo.back = depthInfo.front;
		depthInfo.minDepthBounds = 0;
		depthInfo.maxDepthBounds = 1;

		std::vector<IRender::Resource::DrawCallDescription::BufferRange>& bufferResources = drawCall->cacheDescription.bufferResources;
		uint32_t binding = 0;
		std::vector<VkVertexInputBindingDescription> inputBindingDescriptions;
		std::vector<VkVertexInputAttributeDescription> inputAttributeDescriptions;
		uint32_t location = 0;

		for (size_t k = 0; k < bufferResources.size(); k++) {
			IRender::Resource::DrawCallDescription::BufferRange& bufferRange = bufferResources[k];
			ResourceImplVulkan<IRender::Resource::BufferDescription>* buffer = static_cast<ResourceImplVulkan<IRender::Resource::BufferDescription>*>(bufferRange.buffer);
			IRender::Resource::BufferDescription& desc = buffer->cacheDescription;

			uint32_t bindingIndex = drawCall->GetVertexBufferBindingIndex(k);
			if (k == bindingIndex) {
				VkVertexInputBindingDescription bindingDesc = {};
				bindingDesc.binding = inputBindingDescriptions.size();
				assert(bindingDesc.binding == k);
				bindingDesc.inputRate = desc.usage == IRender::Resource::BufferDescription::INSTANCED ? VK_VERTEX_INPUT_RATE_INSTANCE : VK_VERTEX_INPUT_RATE_VERTEX;
				bindingDesc.stride = desc.stride == 0 ? ComputeBufferStride(desc) : desc.stride;
				inputBindingDescriptions.emplace_back(bindingDesc);
			}

			// process component > 4
			for (uint32_t n = 0; n < desc.component; n += 4) {
				VkVertexInputAttributeDescription attributeDesc = {};
				attributeDesc.location = location++;
				attributeDesc.binding = bindingIndex;
				attributeDesc.format = TranslateFormat(n + 4 >= desc.component ? desc.component % 4 : 4, desc.format);
				inputAttributeDescriptions.emplace_back(std::move(attributeDesc));
			}
		}

		assert(!inputBindingDescriptions.empty());
		assert(!inputAttributeDescriptions.empty());

		VkPipelineVertexInputStateCreateInfo vertexInfo = {};
		vertexInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
		vertexInfo.vertexBindingDescriptionCount = safe_cast<uint32_t>(inputBindingDescriptions.size());
		vertexInfo.pVertexBindingDescriptions = &inputBindingDescriptions[0];
		vertexInfo.vertexAttributeDescriptionCount = safe_cast<uint32_t>(inputAttributeDescriptions.size());
		vertexInfo.pVertexAttributeDescriptions = &inputAttributeDescriptions[0];

		VkPipelineInputAssemblyStateCreateInfo iaInfo = {};
		iaInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
		iaInfo.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;

		VkPipelineViewportStateCreateInfo viewportInfo = {};
		viewportInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
		viewportInfo.viewportCount = 1;
		viewportInfo.scissorCount = 1;

		VkPipelineRasterizationStateCreateInfo rasterInfo = {};
		rasterInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
		rasterInfo.polygonMode = renderState.fill ? VK_POLYGON_MODE_FILL : VK_POLYGON_MODE_LINE;
		rasterInfo.cullMode = renderState.cull ? renderState.cullFrontFace ? VK_CULL_MODE_FRONT_BIT : VK_CULL_MODE_BACK_BIT : VK_CULL_MODE_NONE;
		rasterInfo.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
		rasterInfo.lineWidth = 1.0f;

		VkPipelineMultisampleStateCreateInfo msInfo = {};
		msInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
		// TODO: MSAA
		msInfo.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

		VkPipelineColorBlendStateCreateInfo blendInfo;
		blendInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
		blendInfo.attachmentCount = safe_cast<uint32_t>(target->cacheDescription.colorStorages.size());
		assert(blendInfo.attachmentCount != 0);
		// TODO: different blend operations
		// create state instance
		VkPipelineColorBlendAttachmentState blendState = {};
		blendState.blendEnable = renderState.blend;
		if (blendState.blendEnable) {
			blendState.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
			blendState.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
			blendState.colorBlendOp = VK_BLEND_OP_ADD;
			blendState.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
			blendState.dstColorBlendFactor = VK_BLEND_FACTOR_ZERO;
			blendState.alphaBlendOp = VK_BLEND_OP_ADD;
			blendState.colorWriteMask = renderState.colorWrite ? VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT : 0;
		}
		std::vector<VkPipelineColorBlendAttachmentState> blendStates(blendInfo.attachmentCount, blendState);
		blendInfo.pAttachments = &blendStates[0];

		VkDynamicState dynamicStates[2] = { VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR };
		VkPipelineDynamicStateCreateInfo dynamicState = {};
		dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
		dynamicState.dynamicStateCount = sizeof(dynamicStates) / sizeof(dynamicStates[0]);
		dynamicState.pDynamicStates = dynamicStates;

		VkGraphicsPipelineCreateInfo info = {};
		info.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
		info.flags = 0;
		info.stageCount = safe_cast<uint32_t>(shaderStageCreateInfos.size());
		info.pStages = &shaderStageCreateInfos[0];
		info.pVertexInputState = &vertexInfo;
		info.pViewportState = &viewportInfo;
		info.pRasterizationState = &rasterInfo;
		info.pMultisampleState = &msInfo;
		info.pDepthStencilState = &depthInfo;
		info.pColorBlendState = &blendInfo;
		info.pDynamicState = &dynamicState;
		info.layout = pipelineLayout;
		info.renderPass = target->renderPass;

		PipelineInstance instance;
		VulkanDeviceImpl* device = queue->device;
		Verify("create graphics pipelines", vkCreateGraphicsPipelines(device->device, VK_NULL_HANDLE, 1, &info, device->allocator, &instance.pipeline));
	}

	virtual void Delete(VulkanQueueImpl* queue) override {
		Clear(queue->device);
	}

	virtual void Upload(VulkanQueueImpl* queue, IRender::Resource::Description* d) override {
		IRender::Resource::ShaderDescription& pass = *static_cast<IRender::Resource::ShaderDescription*>(d);
		VulkanDeviceImpl* device = queue->device;

		std::vector<IShader*> shaders[Resource::ShaderDescription::END];
		String common;
		for (size_t i = 0; i < pass.entries.size(); i++) {
			const std::pair<Resource::ShaderDescription::Stage, IShader*>& component = pass.entries[i];

			if (component.first == Resource::ShaderDescription::GLOBAL) {
				common += component.second->GetShaderText();
			} else {
				shaders[component.first].emplace_back(component.second);
			}
		}

		std::vector<VkDescriptorSetLayoutBinding> textureBindings;
		std::vector<VkSampler> samplers;

		for (size_t k = 0; k < Resource::ShaderDescription::END; k++) {
			std::vector<IShader*>& pieces = shaders[k];
			if (pieces.empty()) continue;

			String body = "void main(void) {\n";
			String head = "";
			uint32_t inputIndex = 0, outputIndex = 0, textureIndex = 0;
			for (size_t n = 0; n < pieces.size(); n++) {
				IShader* shader = pieces[n];
				// Generate declaration
				GLSLShaderGenerator declaration((Resource::ShaderDescription::Stage)k, inputIndex, outputIndex, textureIndex);
				(*shader)(declaration);
				declaration.Complete();

				body += declaration.initialization + shader->GetShaderText() + declaration.finalization + "\n";
				head += declaration.declaration;

				for (size_t k = 0; k < declaration.bufferBindings.size(); k++) {
					std::pair<const IShader::BindBuffer*, String>& item = declaration.bufferBindings[k];
					const IShader::BindBuffer* bindBuffer = item.first;
					bufferDescriptions.emplace_back(bindBuffer->description);
				}

				size_t startSamplerIndex = samplers.size();
				for (size_t m = 0; m < declaration.textureBindings.size(); m++) {
					std::pair<const IShader::BindTexture*, String>& item = declaration.textureBindings[m];
					const IRender::Resource::TextureDescription::State& state = item.first->description.state;

					VkSamplerCreateInfo info = {};
					info.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
					info.minFilter = info.magFilter = state.sample == IRender::Resource::TextureDescription::POINT ? VK_FILTER_NEAREST : VK_FILTER_LINEAR;
					info.mipmapMode = VK_SAMPLER_MIPMAP_MODE_NEAREST; // not tri-filter
					info.addressModeV = info.addressModeU = info.addressModeW = state.wrap ? VK_SAMPLER_ADDRESS_MODE_REPEAT : VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
					info.minLod = -1000;
					info.maxLod = 1000;
					info.maxAnisotropy = 4.0f;
					info.anisotropyEnable = state.sample == IRender::Resource::TextureDescription::ANSOTRIPIC;

					VkSampler sampler;
					Verify("create sampler", vkCreateSampler(device->device, &info, device->allocator, &sampler));
					samplers.emplace_back(sampler);
				}

				if (!declaration.textureBindings.empty()) {
					VkDescriptorSetLayoutBinding binding;
					binding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
					binding.descriptorCount = (uint32_t)declaration.textureBindings.size();
					binding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
					switch (k) {
					case IRender::Resource::ShaderDescription::VERTEX:
						binding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
						break;
					case IRender::Resource::ShaderDescription::TESSELLATION_CONTROL:
						binding.stageFlags = VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT;
						break;
					case IRender::Resource::ShaderDescription::TESSELLATION_EVALUATION:
						binding.stageFlags = VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT;
						break;
					case IRender::Resource::ShaderDescription::GEOMETRY:
						binding.stageFlags = VK_SHADER_STAGE_GEOMETRY_BIT;
						break;
					case IRender::Resource::ShaderDescription::FRAGMENT:
						binding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
						break;
					case IRender::Resource::ShaderDescription::COMPUTE:
						binding.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
						break;
					}

					binding.pImmutableSamplers = reinterpret_cast<VkSampler*>(startSamplerIndex);
					textureBindings.emplace_back(binding);
				}
			}

			body += "\n}\n"; // make a call to our function

			String fullShader = GLSLShaderGenerator::GetFrameCode() + common + head + body;

			// TODO: fill spirv
			VkShaderModuleCreateInfo createInfo = {};
			createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
			createInfo.codeSize = fullShader.size();
			createInfo.pCode = reinterpret_cast<const uint32_t*>(fullShader.c_str());

			Verify("create shader module", vkCreateShaderModule(device->device, &createInfo, device->allocator, &shaderModules[k]));

			if (pass.compileCallback) {
				pass.compileCallback(pass, (IRender::Resource::ShaderDescription::Stage)k, "", fullShader);
			}

			VkPipelineShaderStageCreateInfo shaderStageInfo = {};
			shaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
			shaderStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
			shaderStageInfo.module = shaderModules[k];
			shaderStageInfo.pName = "main";

			switch (k) {
			case IRender::Resource::ShaderDescription::VERTEX:
				shaderStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
				break;
			case IRender::Resource::ShaderDescription::TESSELLATION_CONTROL:
				shaderStageInfo.stage = VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT;
				break;
			case IRender::Resource::ShaderDescription::TESSELLATION_EVALUATION:
				shaderStageInfo.stage = VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT;
				break;
			case IRender::Resource::ShaderDescription::GEOMETRY:
				shaderStageInfo.stage = VK_SHADER_STAGE_GEOMETRY_BIT;
				break;
			case IRender::Resource::ShaderDescription::FRAGMENT:
				shaderStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
				break;
			case IRender::Resource::ShaderDescription::COMPUTE:
				shaderStageInfo.stage = VK_SHADER_STAGE_COMPUTE_BIT;
				break;
			}

			shaderStageCreateInfos.emplace_back(std::move(shaderStageInfo));
		}

		// fix sampler pointers
		for (size_t j = 0; j < textureBindings.size(); j++) {
			textureBindings[j].pImmutableSamplers = &samplers[*(uint32_t*)&textureBindings[j].pImmutableSamplers];
		}

		if (!textureBindings.empty()) {
			VkDescriptorSetLayoutCreateInfo info = {};
			info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
			info.bindingCount = (uint32_t)textureBindings.size();
			info.pBindings = &textureBindings[0];
			Verify("create descriptor set layout", vkCreateDescriptorSetLayout(device->device, &info, device->allocator, &descriptorSetLayout));

			VkDescriptorSetAllocateInfo allocInfo = {};
			allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
			allocInfo.descriptorPool = device->descriptorPool;
			allocInfo.descriptorSetCount = 1;
			allocInfo.pSetLayouts = &descriptorSetLayout;
			Verify("allocate descriptor set", vkAllocateDescriptorSets(device->device, &allocInfo, &descriptorSet));
		}

		// TODO: push constant range
		// VkPushConstantRange 
		VkPipelineLayoutCreateInfo layoutInfo = {};
		layoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
		layoutInfo.setLayoutCount = descriptorSetLayout ? 1 : 0;
		layoutInfo.pSetLayouts = &descriptorSetLayout;
		layoutInfo.pushConstantRangeCount = 0;
		layoutInfo.pPushConstantRanges = nullptr;
		Verify("create pipeline layout", vkCreatePipelineLayout(device->device, &layoutInfo, device->allocator, &pipelineLayout));
	}

	virtual void Execute(VulkanQueueImpl* queue) override {
		assert(false); // not possible
	}

	VkDescriptorSetLayout descriptorSetLayout;
	VkDescriptorSet descriptorSet;
	VkPipelineLayout pipelineLayout;
	VkShaderModule shaderModules[IRender::Resource::ShaderDescription::Stage::END];

	std::vector<VkPipelineShaderStageCreateInfo> shaderStageCreateInfos;
	std::vector<IRender::Resource::BufferDescription> bufferDescriptions;
	std::vector<std::key_value<PipelineKey, PipelineInstance> > stateInstances;
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
void ZRenderVulkan::SetResourceNotation(Resource* lhs, const String& note) {

}

void ZRenderVulkan::DeleteResource(Queue* queue, Resource* resource) {
	VulkanQueueImpl* impl = static_cast<VulkanQueueImpl*>(queue);
	impl->deletedResources.Push(static_cast<ResourceImplVulkanBase*>(resource));
}

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
