#include "VulkanContext.h"
#include <spdlog/spdlog.h>
#include <SDL2/SDL_vulkan.h>
#define VMA_IMPLEMENTATION
#include "vma/vk_mem_alloc.h"
#include <iostream>
#include <set>
#include <algorithm>

VGM::VulkanContext::VulkanContext(SDL_Window* window, uint32_t windowWidth, uint32_t windowHeight, uint32_t apiVerionsMajor, uint32_t apiVersionMinor, uint32_t apiVersionPatch, std::vector<const char*> requiredInstanceExtensions, std::vector<const char*> requiredDeviceExtensions)
{
	vkb::InstanceBuilder instanceBuilder;
	instanceBuilder.require_api_version(apiVerionsMajor, apiVersionMinor, apiVersionPatch);
	for (auto& e : requiredInstanceExtensions)
	{
		instanceBuilder.enable_extension(e);
	}
	instanceBuilder.set_app_version(VK_MAKE_API_VERSION(1, 0, 0, 0))
		.set_app_name("VGM")
		.request_validation_layers(true)
		.add_debug_messenger_severity(VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT)
		.use_default_debug_messenger()
		.require_api_version(apiVerionsMajor, apiVersionMinor, apiVersionPatch);
		
	auto retInstance = instanceBuilder.build();
	vkb::Instance instance = retInstance.value();

#ifdef _DEBUG
	_debugMessenger = instance.debug_messenger;
#endif // DEBUG

	std::cout << "about to create surface"<<std::endl;
	SDL_Vulkan_CreateSurface(window, instance.instance, &_surface);
	int width, height;
	SDL_GetWindowSize(window, &width, &height);

	VkPhysicalDeviceVulkan13Features features{};
	
	if(apiVerionsMajor == 1 && apiVersionMinor == 3)
		features.dynamicRendering = true;

	std::cout << "about to pick physical device" << std::endl;
	vkb::PhysicalDeviceSelector physDeviceSelector{ instance };
	physDeviceSelector.add_desired_extensions(requiredDeviceExtensions)
		.set_surface(_surface)
		.set_minimum_version(apiVerionsMajor, apiVersionMinor);
		physDeviceSelector.set_required_features_13(features);
	auto retPhysDevice = physDeviceSelector.select();
	vkb::PhysicalDevice physicalDevice = retPhysDevice.value();

	std::cout << "about to build logical device" << std::endl;
	vkb::DeviceBuilder deviceBuilder{ physicalDevice };
	auto retDevice = deviceBuilder.build();
	vkb::Device device = retDevice.value();

	std::cout << "about to build swapchain" << std::endl;
	vkb::SwapchainBuilder swapchainBuilder(physicalDevice, device, _surface);
	swapchainBuilder.use_default_format_selection()
		.set_desired_format({VK_FORMAT_R8G8B8A8_UNORM, VK_COLOR_SPACE_SRGB_NONLINEAR_KHR })
		.set_desired_present_mode(VK_PRESENT_MODE_FIFO_KHR)
		.set_desired_min_image_count(vkb::SwapchainBuilder::DOUBLE_BUFFERING)
		.set_desired_extent(windowWidth, windowHeight);

	auto retSwapchain = swapchainBuilder.build();
	vkb::Swapchain swapchain = retSwapchain.value();

	auto graphicsQueue = device.get_queue(vkb::QueueType::graphics);
	uint32_t graphicsQueueIndex = device.get_queue_index(vkb::QueueType::graphics).value();

	auto presentQueue = device.get_queue(vkb::QueueType::present);
	uint32_t presentQueueIndex = device.get_queue_index(vkb::QueueType::present).value();

	auto transferQueue = device.get_queue(vkb::QueueType::transfer);
	uint32_t transferQueueIndex = device.get_queue_index(vkb::QueueType::transfer).value();
	
	std::cout << "about to init rest" << std::endl;
	_instance = instance.instance;
	_physicalDevice = physicalDevice.physical_device;
	_phyiscalDeviceProperties = physicalDevice.properties;
	_device = device.device;
	_graphicsQeueu = graphicsQueue.value();
	_presentQueue = presentQueue.value();
	_transferQeueu = transferQueue.value();
	_graphicsQueueFamilyIndex = graphicsQueueIndex;
	_presentQueueFamilyIndex = presentQueueIndex;
	_transferQueueFamilyIndex = transferQueueIndex;
	
	_swapchain = swapchain.swapchain;
	_swapchainWidth = windowWidth;
	_swapchainHeight = windowHeight;
	_swapchainImages = swapchain.get_images().value();
	_swapchainImageViews = swapchain.get_image_views().value();
	_swapchainFormat.format = swapchain.image_format;
	_swapchainFormat.colorSpace = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR;

	VmaAllocatorCreateInfo allocCreateInfo = {};
	allocCreateInfo.instance = _instance;
	allocCreateInfo.physicalDevice = _physicalDevice;
	allocCreateInfo.device = _device;
	allocCreateInfo.vulkanApiVersion = VK_MAKE_API_VERSION(0, apiVerionsMajor, apiVersionMinor, apiVersionPatch);
	
	std::cout << "about init allocator" << std::endl;
	vmaCreateAllocator(&allocCreateInfo, &_gpuAllocator);

	VkSemaphoreCreateInfo createInfo = {};
	createInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
	createInfo.pNext = nullptr;
	createInfo.flags = 0;

	vkCreateSemaphore(_device, &createInfo, nullptr, &_presentSemaphore);
}

VGM::VulkanContext::VulkanContext(SDL_Window* window, uint32_t windowWidth, uint32_t windowHeight, uint32_t apiVerionsMajor, uint32_t apiVersionMinor, uint32_t apiVersionPatch, std::vector<const char*> requiredInstanceExtensions, std::vector<const char*> requiredValidationLayers, std::vector<const char*> requiredDeviceExtensions, const void* devicePnextChain, uint32_t swapchainSize)
{
	unsigned int sdlExtensionCount = 0;
	SDL_Vulkan_GetInstanceExtensions(window, &sdlExtensionCount, nullptr);
	std::vector<const char*> sdlExtensions(sdlExtensionCount);
	SDL_Vulkan_GetInstanceExtensions(window, &sdlExtensionCount, sdlExtensions.data());

	requiredInstanceExtensions.insert(requiredInstanceExtensions.end(), sdlExtensions.begin(), sdlExtensions.end());

	VkResult result = createInstance(requiredInstanceExtensions, requiredValidationLayers, apiVerionsMajor, apiVersionMinor, apiVersionPatch);
	if (result == VK_ERROR_EXTENSION_NOT_PRESENT)
		throw std::runtime_error("Extension not present!");
	if (result == VK_ERROR_LAYER_NOT_PRESENT)
		throw std::runtime_error("Layer not present!");
	if (result != VK_SUCCESS)
		throw std::runtime_error("Failed to create instance!");

	std::cout << "Created Instance successfully!" << std::endl;

#ifdef _DEBUG
	result = createDebugMessenger(VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT,
		VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
		VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
		VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT);
	if (result == VK_ERROR_EXTENSION_NOT_PRESENT)
		throw std::runtime_error("Debug messenger extension not present!");

	std::cout << "Created Debug Messenger successfully!" << std::endl;
#endif // _DEBUG

	


	if (SDL_Vulkan_CreateSurface(window, _instance, &_surface) == SDL_FALSE)
		throw std::runtime_error("Failed to create Surface!");

	std::cout << "Created Surface successfully!" << std::endl;

	result = pickPhysicalDevice(requiredDeviceExtensions, { VK_QUEUE_GRAPHICS_BIT, VK_QUEUE_TRANSFER_BIT, VK_QUEUE_COMPUTE_BIT }, VK_MAKE_API_VERSION(0, apiVerionsMajor, apiVersionMinor, apiVersionPatch));
	if (result != VK_SUCCESS)
		throw std::runtime_error("Failed to pick Physical Device!");

	std::cout << "Created Physical Device successfully!" << std::endl;

	result = createLogicalDevice({ VK_QUEUE_GRAPHICS_BIT, VK_QUEUE_TRANSFER_BIT, VK_QUEUE_COMPUTE_BIT }, requiredDeviceExtensions, requiredValidationLayers, devicePnextChain);
	if (result != VK_SUCCESS)
		throw std::runtime_error("Failed to create logical Device!");

	std::cout << "Created Logical Device successfully!" << std::endl;

	getDeviceQueues();

	std::cout << "Created Queues successfully!" << std::endl;

	int width = 0;
	int height = 0;

	SDL_Vulkan_GetDrawableSize(window, &width, &height);

	std::vector<VkSurfaceFormatKHR> surfaceFormats;
	surfaceFormats.push_back({ VK_FORMAT_B8G8R8A8_UNORM,  VK_COLOR_SPACE_SRGB_NONLINEAR_KHR });
	result = createSwapchain(surfaceFormats, VK_PRESENT_MODE_FIFO_KHR, { (uint32_t)width, (uint32_t)height }, swapchainSize);
	if (result != VK_SUCCESS)
		throw std::runtime_error("Failed to create Swapchain!");

	std::cout << "Created Swapchain successfully!" << std::endl;

	result = createSwapchainImageViews();
	if (result != VK_SUCCESS)
		throw std::runtime_error("Failed to create swapchain Image-Views!");

	std::cout << "Created Swapchain views successfully!" << std::endl;

	VmaAllocatorCreateInfo allocCreateInfo = {};
	allocCreateInfo.instance = _instance;
	allocCreateInfo.physicalDevice = _physicalDevice;
	allocCreateInfo.device = _device;
	allocCreateInfo.flags = VMA_ALLOCATOR_CREATE_BUFFER_DEVICE_ADDRESS_BIT;
	allocCreateInfo.vulkanApiVersion = VK_MAKE_API_VERSION(0, apiVerionsMajor, apiVersionMinor, apiVersionPatch);

	result = vmaCreateAllocator(&allocCreateInfo, &_gpuAllocator);
	if (result != VK_SUCCESS)
		throw std::runtime_error("Failed to create gpuMemoryAllocator!");

	result = vmaCreateAllocator(&allocCreateInfo, &_generalPurposeAllocator);
	if (result != VK_SUCCESS)
		throw std::runtime_error("Failed to create gpuMemoryAllocator!");

	result = vmaCreateAllocator(&allocCreateInfo, &_textureAllocator);
	if (result != VK_SUCCESS)
		throw std::runtime_error("Failed to create gpuMemoryAllocator!");

	result = vmaCreateAllocator(&allocCreateInfo, &_meshAllocator);
	if (result != VK_SUCCESS)
		throw std::runtime_error("Failed to create gpuMemoryAllocator!");

	VkSemaphoreCreateInfo createInfo = {};
	createInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
	createInfo.pNext = nullptr;
	createInfo.flags = 0;

	result = vkCreateSemaphore(_device, &createInfo, nullptr, &_presentSemaphore);
	if (result != VK_SUCCESS)
		throw std::runtime_error("Failed to create presentSemaphore!");

	std::cout << "Created Semaphore successfully!" << std::endl;

	vkGetPhysicalDeviceProperties(_physicalDevice, &_phyiscalDeviceProperties);
}

uint32_t VGM::VulkanContext::getSwapchainSize()
{
	return static_cast<uint32_t>(_swapchainImages.size());
}

void VGM::VulkanContext::rebuildSwapchain(int width, int height)
{
	vkDeviceWaitIdle(_device);
	
	destroySwapchain();

	vkb::SwapchainBuilder swapchainBuilder(_physicalDevice, _device, _surface);
	swapchainBuilder.use_default_format_selection()
		.set_desired_present_mode(VK_PRESENT_MODE_FIFO_KHR)
		.set_desired_format(_swapchainFormat)
		.set_desired_min_image_count(vkb::SwapchainBuilder::TRIPLE_BUFFERING)
		.set_desired_extent(width, height);

	auto retSwapchain = swapchainBuilder.build();
	vkb::Swapchain swapchain = retSwapchain.value();

	_swapchain = swapchain.swapchain;
	_swapchainWidth = width;
	_swapchainHeight = height;
	_swapchainImages = swapchain.get_images().value();
	_swapchainImageViews = swapchain.get_image_views().value();
	_swapchainFormat.format = swapchain.image_format;
}

void VGM::VulkanContext::recreateSwapchain(uint32_t width, uint32_t height)
{
	uint32_t swapchainSize = _swapchainImages.size();
	destroySwapchain();

	VkExtent2D extends = { width, height };

	std::vector<VkSurfaceFormatKHR> formats = { _swapchainFormat };
	createSwapchain(formats, _presentMode, extends, swapchainSize);

	createSwapchainImageViews();
}

uint32_t VGM::VulkanContext::aquireNextImageIndex(VkFence waitFence, VkSemaphore singalSemaphore)
{
	vkAcquireNextImageKHR(_device, _swapchain, 1000000000, singalSemaphore, waitFence, &_currentImageIndex);
	return _currentImageIndex;
}

void VGM::VulkanContext::cmdPrepareSwapchainImageForRendering(VkCommandBuffer commandBuffer)
{
	VkImageMemoryBarrier memBarrier = {};
	memBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
	memBarrier.pNext = nullptr;
	memBarrier.image = _swapchainImages[_currentImageIndex];
	memBarrier.subresourceRange.baseArrayLayer = 0;
	memBarrier.subresourceRange.baseMipLevel = 0;
	memBarrier.subresourceRange.layerCount = 1;
	memBarrier.subresourceRange.levelCount = 1;
	memBarrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	memBarrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	memBarrier.newLayout = VK_IMAGE_LAYOUT_ATTACHMENT_OPTIMAL;
	memBarrier.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

	vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, 0, 0, nullptr, 0, nullptr, 1, &memBarrier);
}

void VGM::VulkanContext::cmdPrepareSwaphainImageForPresent(VkCommandBuffer commandBuffer)
{
	VkImageMemoryBarrier memBarrier = {};
	memBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
	memBarrier.pNext = nullptr;
	memBarrier.image = _swapchainImages[_currentImageIndex];
	memBarrier.subresourceRange.baseArrayLayer = 0;
	memBarrier.subresourceRange.baseMipLevel = 0;
	memBarrier.subresourceRange.layerCount = 1;
	memBarrier.subresourceRange.levelCount = 1;
	memBarrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	memBarrier.oldLayout = VK_IMAGE_LAYOUT_ATTACHMENT_OPTIMAL;
	memBarrier.newLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
	memBarrier.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
	memBarrier.dstAccessMask = VK_ACCESS_NONE;

	vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, 0, 0, nullptr, 0, nullptr, 1, &memBarrier);
}

void VGM::VulkanContext::presentImage(VkSemaphore* pWaitSemaphores, uint32_t waitSemaphoreCount)
{
	VkPresentInfoKHR presentInfo = {};
	presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
	presentInfo.pNext = nullptr;
	presentInfo.pSwapchains = &_swapchain;
	presentInfo.swapchainCount = 1;
	presentInfo.pImageIndices = &_currentImageIndex;
	presentInfo.pWaitSemaphores = pWaitSemaphores;
	presentInfo.waitSemaphoreCount = waitSemaphoreCount;
	
	vkQueuePresentKHR(_graphicsQeueu, &presentInfo);
}

void VGM::VulkanContext::destroySwapchain()
{
	for(auto& i : _swapchainImageViews)
	{
		vkDestroyImageView(_device, i, nullptr);
	}

	vkDestroySwapchainKHR(_device, _swapchain, nullptr);
}

VkResult createDebugMessengerFunction(VkInstance instance, const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo,
	const VkAllocationCallbacks* pAllocator, VkDebugUtilsMessengerEXT* pDebugMessenger)
{
	auto func = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT");
	if (func == nullptr)
		return VK_ERROR_EXTENSION_NOT_PRESENT;
	return func(instance, pCreateInfo, pAllocator, pDebugMessenger);
}

VkResult destroyDebugMessengerFunction(VkInstance instance, const VkAllocationCallbacks* pAllocator, VkDebugUtilsMessengerEXT DebugMessenger)
{
	auto func = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT");
	if (func == nullptr)
		return VK_ERROR_EXTENSION_NOT_PRESENT;
	func(instance, DebugMessenger, pAllocator);
	return VK_SUCCESS;
}

void VGM::VulkanContext::destroy()
{
	vmaDestroyAllocator(_gpuAllocator);
	destroySwapchain();
	vkDestroySurfaceKHR(_instance, _surface, nullptr);
	vkDestroyDevice(_device, nullptr);

	destroyDebugMessengerFunction(_instance, nullptr, _debugMessenger);
	
	vkDestroyInstance(_instance, nullptr);
}

VkResult VGM::VulkanContext::createInstance(std::vector<const char*>& requiredExtensions, std::vector<const char*>& rquiredValidationLayers, uint32_t versionMajor, uint32_t versionMinor, uint32_t versionPatch)
{
	if(rquiredValidationLayers.size() != 0)
	{
		requiredExtensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
	}

	if (checkInstanceExtensions(requiredExtensions) == VK_ERROR_EXTENSION_NOT_PRESENT)
		return VK_ERROR_EXTENSION_NOT_PRESENT;
	
	if (checkInstanceValidationLayers(rquiredValidationLayers) == VK_ERROR_LAYER_NOT_PRESENT)
		return VK_ERROR_LAYER_NOT_PRESENT;

	VkApplicationInfo appInfo = {};
	appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
	appInfo.pNext = nullptr;
	appInfo.applicationVersion = 1;
	appInfo.engineVersion = 1;
	appInfo.pApplicationName = "VGM";
	appInfo.pEngineName = "VGM";
	appInfo.apiVersion = VK_MAKE_API_VERSION(0, versionMajor, versionMinor, versionPatch);

	VkInstanceCreateInfo createInfo = {};
	createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
	createInfo.pNext = nullptr;
	createInfo.pApplicationInfo = &appInfo;
	createInfo.enabledExtensionCount = requiredExtensions.size();
	createInfo.ppEnabledExtensionNames = requiredExtensions.data();
	createInfo.enabledLayerCount = rquiredValidationLayers.size();
	createInfo.ppEnabledLayerNames = rquiredValidationLayers.data();
	
	return vkCreateInstance(&createInfo, nullptr, &_instance);
}

VkResult VGM::VulkanContext::checkInstanceExtensions(std::vector<const char*>& requiredExtensions)
{
	uint32_t extensionCount;
	vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, nullptr);
	std::vector<VkExtensionProperties> extensionProperties(extensionCount);
	vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, extensionProperties.data());

	for (unsigned int i = 0; i < requiredExtensions.size(); i++)
	{
		bool extensionPresent = false;
		for (unsigned int j = 0; j < extensionCount; j++)
		{
			if (std::strcmp(requiredExtensions[i], extensionProperties[j].extensionName) == 0)
			{
				extensionPresent = true;
			}
		}
		if (!extensionPresent)
		{
			return VK_ERROR_EXTENSION_NOT_PRESENT;
		}
	}

	return VK_SUCCESS;
}

VkResult VGM::VulkanContext::checkInstanceValidationLayers(std::vector<const char*>& rquiredValidationLayers)
{
	uint32_t layerCount;
	vkEnumerateInstanceLayerProperties(&layerCount, nullptr);
	std::vector<VkLayerProperties> layerProperties(layerCount);
	vkEnumerateInstanceLayerProperties(&layerCount, layerProperties.data());

	for (unsigned int i = 0; i < rquiredValidationLayers.size(); i++)
	{
		bool layPresent = false;
		for (unsigned int j = 0; j < layerCount; j++)
		{
			if (std::strcmp(rquiredValidationLayers[i], layerProperties[j].layerName) == 0)
			{
				layPresent = true;
			}
		}
		if (!layPresent)
		{
			return VK_ERROR_LAYER_NOT_PRESENT;
		}
	}

	return VK_SUCCESS;
}


VkResult VGM::VulkanContext::createDebugMessenger(VkDebugUtilsMessageSeverityFlagsEXT severityFlags, VkDebugUtilsMessageTypeFlagsEXT typeFlags)
{
	VkDebugUtilsMessengerCreateInfoEXT createInfo = {};
	createInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
	createInfo.pNext = nullptr;
	createInfo.messageSeverity = severityFlags;
	createInfo.messageType = typeFlags;
	createInfo.pUserData = nullptr;
	createInfo.pfnUserCallback = debugCallback;

	return createDebugMessengerFunction(_instance, &createInfo, nullptr, &_debugMessenger);
}

VKAPI_ATTR VkBool32 VKAPI_CALL VGM::VulkanContext::debugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity, VkDebugUtilsMessageTypeFlagsEXT messageType, const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData, void* pUserData)
{
	std::cerr << "Validation layer:" << pCallbackData->pMessage << std::endl;
	return VK_FALSE;
}

VkResult VGM::VulkanContext::pickPhysicalDevice(std::vector<const char*> requiredDeviceExtensions, std::vector<VkQueueFlagBits> desiredQueues, uint32_t requiredAPIVerison)
{
	uint32_t physicalDeviceCount = 0;
	vkEnumeratePhysicalDevices(_instance, &physicalDeviceCount, nullptr);
	std::vector<VkPhysicalDevice> physicalDevices(physicalDeviceCount);
	vkEnumeratePhysicalDevices(_instance, &physicalDeviceCount, physicalDevices.data());

	_physicalDevice = physicalDevices[0];
	return VK_SUCCESS;

	std::vector<VkPhysicalDevice> suitableDevcies;
	
	for(unsigned int i = 0; i<physicalDeviceCount; i++)
	{
		if (checkPQueueFamilies(desiredQueues, physicalDevices[i]) == VK_SUCCESS &&
			checkDeviceExtenions(requiredDeviceExtensions, physicalDevices[i]) == VK_SUCCESS)
		{
			suitableDevcies.push_back(physicalDevices[i]);
		}
	}

	if (suitableDevcies.empty())
	{
		return VK_ERROR_FEATURE_NOT_PRESENT;
	}

	std::vector<uint32_t> physcialDeviceScores(suitableDevcies.size(), 0);
	for(unsigned int i = 0; i<suitableDevcies.size(); i++)
	{
		VkPhysicalDeviceProperties properties = {};
		vkGetPhysicalDeviceProperties(suitableDevcies[i], &properties);

		if(properties.apiVersion >= requiredAPIVerison &&
			properties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU)
		{
			uint32_t score;
			score = properties.limits.maxBoundDescriptorSets +
				properties.limits.maxColorAttachments +
				properties.limits.maxDescriptorSetInputAttachments +
				properties.limits.maxDescriptorSetSampledImages +
				properties.limits.maxDescriptorSetSamplers +
				properties.limits.maxDescriptorSetStorageBuffers +
				properties.limits.maxDescriptorSetStorageBuffersDynamic +
				properties.limits.maxDescriptorSetStorageImages +
				properties.limits.maxDescriptorSetUniformBuffers +
				properties.limits.maxDescriptorSetUniformBuffersDynamic +
				properties.limits.maxFramebufferHeight +
				properties.limits.maxFramebufferWidth + 
				properties.limits.maxImageDimension2D;

			
			physcialDeviceScores[i] = score;
		}
	}

	uint32_t largestScore = physcialDeviceScores.front();
	uint32_t largesScoreIndex = 0;
	for(unsigned int i = 0; i< physcialDeviceScores.size(); i++)
	{
		if (largestScore < physcialDeviceScores[i])
		{
			largestScore = physcialDeviceScores[i];
			largesScoreIndex = i;
		}
	}

	if(largestScore != 0)
	{
		_physicalDevice = suitableDevcies[largesScoreIndex];
		return VK_SUCCESS;
	}
}

VkResult VGM::VulkanContext::checkPQueueFamilies(std::vector<VkQueueFlagBits> desiredQueues, VkPhysicalDevice physicalDevice)
{
	uint32_t queueFmiliesCount;
	vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueFmiliesCount, nullptr);
	std::vector<VkQueueFamilyProperties> queueFamilyProperties(queueFmiliesCount);
	vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueFmiliesCount, queueFamilyProperties.data());

	for(unsigned int i = 0; i< desiredQueues.size(); i++)
	{
		bool queuePresent = false;
		for(unsigned int j = 0; j<queueFmiliesCount; j++)
		{
			if(desiredQueues[i] & queueFamilyProperties[j].queueFlags)
			{
				queuePresent = true;
			}
		}
		if(!queuePresent)
		{
			return VK_ERROR_FEATURE_NOT_PRESENT;
		}
	}

	return VK_SUCCESS;
}

VkResult VGM::VulkanContext::checkDeviceExtenions(std::vector<const char*> requiredDeviceExtensions, VkPhysicalDevice physicalDevice)
{
	uint32_t extensionCount = 0;
	vkEnumerateDeviceExtensionProperties(physicalDevice, nullptr, &extensionCount, nullptr);
	std::vector<VkExtensionProperties> extenionProperties(extensionCount);
	vkEnumerateDeviceExtensionProperties(physicalDevice, nullptr, &extensionCount, extenionProperties.data());

	for (unsigned int i = 0; i < requiredDeviceExtensions.size(); i++)
	{
		bool layPresent = false;
		for (unsigned int j = 0; j < extensionCount; j++)
		{
			if (std::strcmp(requiredDeviceExtensions[i], extenionProperties[j].extensionName) == 0)
			{
				layPresent = true;
			}
		}
		if (!layPresent)
		{
			return VK_ERROR_EXTENSION_NOT_PRESENT;
		}
	}

	return VK_SUCCESS;
}

void VGM::VulkanContext::getQueueFamilyIndices(std::vector<VkQueueFlagBits> desiredQueues, VkPhysicalDevice physicalDevice, std::multimap<VkQueueFlagBits, uint32_t>& indicesMap)
{
	uint32_t queueFmiliesCount;
	vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueFmiliesCount, nullptr);
	std::vector<VkQueueFamilyProperties> queueFamilyProperties(queueFmiliesCount);
	vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueFmiliesCount, queueFamilyProperties.data());

	for (unsigned int i = 0; i < desiredQueues.size(); i++)
	{
		for (unsigned int j = 0; j < queueFmiliesCount; j++)
		{
			if (desiredQueues[i] & queueFamilyProperties[j].queueFlags)
			{
				indicesMap.insert(std::make_pair(desiredQueues[i], j));
			}
		}
	}
}

VkResult VGM::VulkanContext::getQueueFamilyIndex(VkPhysicalDevice physicalDevice, VkQueueFlagBits desiredQueue, uint32_t& QueueFamilyIndex)
{
	uint32_t familyCount = 0;
	vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &familyCount, nullptr);
	std::vector<VkQueueFamilyProperties> properties(familyCount);
	vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &familyCount, properties.data());

	uint32_t i = 0;
	for(unsigned int i = 0; properties.size(); i++)
	{
		if(desiredQueue & properties[i].queueFlags && properties[i].timestampValidBits)
		{
			QueueFamilyIndex = i;
			return VK_SUCCESS;
		}
		i++;
	}

	return VK_ERROR_FEATURE_NOT_PRESENT;
}

void VGM::VulkanContext::getPresentQueueFamilyIndices(VkPhysicalDevice physicalDevice, VkSurfaceKHR surface, std::vector<uint32_t>& presentQueueFamilyIndices)
{
	uint32_t familyCount = 0;
	vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &familyCount, nullptr);
	std::vector<VkQueueFamilyProperties> properties(familyCount);
	vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &familyCount, properties.data());
	for(unsigned int i = 0; i<properties.size(); i++)
	{
		VkBool32 presentSupport = false;
		vkGetPhysicalDeviceSurfaceSupportKHR(physicalDevice, i, surface, &presentSupport);
		if(presentSupport == VK_TRUE)
		{
			presentQueueFamilyIndices.push_back (i);
		}
	}
}

VkResult VGM::VulkanContext::createLogicalDevice(std::vector<VkQueueFlagBits> desiredQueues, std::vector<const char*>& requiredDeviceExtensions, std::vector<const char*>& rquiredValidationLayers, const void* pNextChain)
{
	std::vector<VkDeviceQueueCreateInfo> deviceQueueCreateInfos;
	float queuePriority = 1.0f;

	std::vector<uint32_t> presentQueueFamilyIndices;
	getPresentQueueFamilyIndices(_physicalDevice, _surface, presentQueueFamilyIndices);
	
	getQueueFamilyIndex(_physicalDevice, VK_QUEUE_GRAPHICS_BIT, _graphicsQueueFamilyIndex);
	getQueueFamilyIndex(_physicalDevice, VK_QUEUE_TRANSFER_BIT, _transferQueueFamilyIndex);
	getQueueFamilyIndex(_physicalDevice, VK_QUEUE_COMPUTE_BIT, _computeQueueFamilyIndex);
		
	_presentQueueFamilyIndex = presentQueueFamilyIndices.front();

	std::set<uint32_t> queueFamilyIndices{ _graphicsQueueFamilyIndex, _transferQueueFamilyIndex, _computeQueueFamilyIndex, _presentQueueFamilyIndex };
	std::vector<uint32_t> queueFamilies;
	queueFamilies.insert(queueFamilies.begin(), queueFamilyIndices.begin(), queueFamilyIndices.end());

	for(auto& q : queueFamilies)
	{
		VkDeviceQueueCreateInfo queueCreateInfo = {};
		queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
		queueCreateInfo.pNext = nullptr;
		queueCreateInfo.pQueuePriorities = &queuePriority;
		queueCreateInfo.queueCount = 1;
		queueCreateInfo.queueFamilyIndex = q;

		deviceQueueCreateInfos.push_back(queueCreateInfo);
	}

	VkDeviceCreateInfo createInfo = {};
	createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
	createInfo.queueCreateInfoCount = deviceQueueCreateInfos.size();
	createInfo.pQueueCreateInfos = deviceQueueCreateInfos.data();
	createInfo.enabledExtensionCount = requiredDeviceExtensions.size();
	createInfo.ppEnabledExtensionNames = requiredDeviceExtensions.data();
	createInfo.enabledLayerCount = rquiredValidationLayers.size();
	createInfo.ppEnabledLayerNames = rquiredValidationLayers.data();
	createInfo.pEnabledFeatures = nullptr;
	createInfo.pNext = pNextChain;
	
	return vkCreateDevice(_physicalDevice, &createInfo, nullptr, &_device);
}

void VGM::VulkanContext::getDeviceQueues()
{
	vkGetDeviceQueue(_device, _graphicsQueueFamilyIndex, 0, &_graphicsQeueu);
	vkGetDeviceQueue(_device, _transferQueueFamilyIndex, 0, &_transferQeueu);
	vkGetDeviceQueue(_device, _presentQueueFamilyIndex, 0, &_presentQueue);
	vkGetDeviceQueue(_device, _computeQueueFamilyIndex, 0, &_computeQueue);
}

VkResult VGM::VulkanContext::createSwapchain(std::vector<VkSurfaceFormatKHR> desiredSurfaceFormats, VkPresentModeKHR desiredPresentMode, VkExtent2D extends, uint32_t swapchainSize)
{
	VkSurfaceCapabilitiesKHR capabilities;
	vkGetPhysicalDeviceSurfaceCapabilitiesKHR(_physicalDevice, _surface, &capabilities);
	
	if(getDesiredSurfaceFormatPresentMode(desiredSurfaceFormats, desiredPresentMode)!=VK_SUCCESS)
		return VK_ERROR_FEATURE_NOT_PRESENT;

	if (swapchainSize > capabilities.maxImageCount && capabilities.maxImageCount > 0)
		swapchainSize = capabilities.maxImageCount;
	if (swapchainSize <= capabilities.minImageCount)
		swapchainSize = capabilities.minImageCount + 1;

	VkSwapchainCreateInfoKHR createInfo = {};
	createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
	createInfo.pNext = nullptr;
	createInfo.imageFormat = _swapchainFormat.format;
	createInfo.imageColorSpace = _swapchainFormat.colorSpace;
	createInfo.imageArrayLayers = 1;
	createInfo.imageExtent = extends;
	createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_STORAGE_BIT;
	createInfo.minImageCount = swapchainSize;
	createInfo.surface = _surface;
	createInfo.oldSwapchain = VK_NULL_HANDLE;
	createInfo.clipped = VK_TRUE;
	createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
	createInfo.preTransform = capabilities.currentTransform;
	createInfo.presentMode = desiredPresentMode;

	_swapchainWidth = extends.width;
	_swapchainHeight = extends.height;
	_presentMode = desiredPresentMode;

	std::set<uint32_t> uniqueQueueFamilyIndices = { _presentQueueFamilyIndex, _graphicsQueueFamilyIndex, 
		_transferQueueFamilyIndex, _computeQueueFamilyIndex };
	std::vector<uint32_t> queueFamilyIndices;
	for (auto& q : uniqueQueueFamilyIndices)
		queueFamilyIndices.push_back(q);
	if(uniqueQueueFamilyIndices.size() > 1)
	{
		createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
		createInfo.pQueueFamilyIndices = queueFamilyIndices.data();
		createInfo.queueFamilyIndexCount = uniqueQueueFamilyIndices.size();
	}
	else
	{
		createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
		createInfo.queueFamilyIndexCount = 0;
		createInfo.pQueueFamilyIndices = nullptr; 
	}

	VkResult result = vkCreateSwapchainKHR(_device, &createInfo, nullptr, &_swapchain);
	if (result != VK_SUCCESS)
		return result;
	
	uint32_t imageCount = 0;
	vkGetSwapchainImagesKHR(_device, _swapchain, &imageCount, nullptr);
	_swapchainImages.resize(imageCount);
	return vkGetSwapchainImagesKHR(_device, _swapchain, &imageCount, _swapchainImages.data());
}

VkResult VGM::VulkanContext::getDesiredSurfaceFormatPresentMode(std::vector<VkSurfaceFormatKHR> desiredSurfaceFormats, VkPresentModeKHR desiredPresentMode)
{
	uint32_t formatCount = 0;
	vkGetPhysicalDeviceSurfaceFormatsKHR(_physicalDevice, _surface, &formatCount, nullptr);
	std::vector<VkSurfaceFormatKHR> formats(formatCount);
	vkGetPhysicalDeviceSurfaceFormatsKHR(_physicalDevice, _surface, &formatCount, formats.data());

	uint32_t presentModeCount = 0;
	vkGetPhysicalDeviceSurfacePresentModesKHR(_physicalDevice, _surface, &presentModeCount, nullptr);
	std::vector< VkPresentModeKHR> presentModes(presentModeCount);
	vkGetPhysicalDeviceSurfacePresentModesKHR(_physicalDevice, _surface, &presentModeCount, presentModes.data());

	if (presentModeCount != 0 && formatCount != 0)
	{
		bool presentModeFound = false;
		for (auto& p : presentModes)
		{
			if (p == desiredPresentMode)
			{
				presentModeFound = true;
				_presentMode = desiredPresentMode;
				break;
			}
		}

		bool surfaceFormatFound = false;
		for (auto& df : desiredSurfaceFormats)
		{
			for (auto& f : formats)
			{
				if (df.format == f.format && df.colorSpace == f.colorSpace)
				{
					surfaceFormatFound = true;
					_swapchainFormat = df;
				}
			}
		}

		if (!presentModeFound || !surfaceFormatFound)
		{
			return VK_ERROR_FEATURE_NOT_PRESENT;
		}

		return VK_SUCCESS;
	}

	return VK_ERROR_FEATURE_NOT_PRESENT;
}

VkResult VGM::VulkanContext::createSwapchainImageViews()
{
	VkImageViewCreateInfo viewCreateInfo = {};
	viewCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
	viewCreateInfo.pNext = nullptr;
	viewCreateInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	viewCreateInfo.subresourceRange.baseArrayLayer = 0;
	viewCreateInfo.subresourceRange.baseMipLevel = 0;
	viewCreateInfo.subresourceRange.layerCount = 1;
	viewCreateInfo.subresourceRange.levelCount = 1;
	viewCreateInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
	viewCreateInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
	viewCreateInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
	viewCreateInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
	viewCreateInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
	viewCreateInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
	viewCreateInfo.format = _swapchainFormat.format;

	for(auto& i : _swapchainImages)
	{
		VkImageView view;
		viewCreateInfo.image = i;
		if(vkCreateImageView(_device, &viewCreateInfo, nullptr, &view) != VK_SUCCESS)	
			return VK_ERROR_INITIALIZATION_FAILED;

		_swapchainImageViews.push_back(view);
	}
	return VK_SUCCESS;
}


