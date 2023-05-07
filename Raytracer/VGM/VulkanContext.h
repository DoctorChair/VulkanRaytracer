#pragma once

//-----------------------------------------------------------------------------
// Purpose: Initializes vulkan instance, swapchain, command queues and validation layers.
//-----------------------------------------------------------------------------

#include <iostream>

#include <vulkan/vulkan.h>
#include <SDL2/SDL.h>
#include <vma/vk_mem_alloc.h>
#include <vk_bootstrap/VkBootstrap.h>
#include <vector>
#include <map>
#include <unordered_map>

namespace VGM
{
	class VulkanContext
	{
	public:
		VulkanContext() = default;
		
		//old
		VulkanContext(SDL_Window* window, uint32_t windowWidth, uint32_t windowHeight, uint32_t apiVerionsMajor, uint32_t apiVersionMinor, uint32_t apiVersionPatch, std::vector<const char*> requiredInstanceExtensions, std::vector<const char*> requiredDeviceExtensions);
		
		VulkanContext(SDL_Window* window, uint32_t windowWidth, uint32_t windowHeight,
			uint32_t apiVerionsMajor,
			uint32_t apiVersionMinor,
			uint32_t apiVersionPatch,
			std::vector<const char*> requiredInstanceExtensions,
			std::vector<const char*> requiredValidationLayers,
			std::vector<const char*> requiredDeviceExtensions, 
			const void* DevicePnextChain, uint32_t swapchainSize);

		uint32_t getSwapchainSize();

		//old
		void rebuildSwapchain(int width, int height);
		void recreateSwapchain(uint32_t width, uint32_t height);

		uint32_t aquireNextImageIndex(VkFence waitFence, VkSemaphore singalSemaphore);
		void cmdPrepareSwapchainImageForRendering(VkCommandBuffer commandBuffer);
		void cmdPrepareSwaphainImageForPresent(VkCommandBuffer commandBuffer);
		void presentImage(VkSemaphore* pWaitSemaphores, uint32_t waitSemaphoreCount);

		void destroySwapchain();
		void destroy();

		VkInstance _instance;
		VkPhysicalDevice _physicalDevice;
		VkPhysicalDeviceProperties _phyiscalDeviceProperties;
		VkDevice _device;
		VkSurfaceKHR _surface;

		VkDebugUtilsMessengerEXT _debugMessenger;


		VkSwapchainKHR _swapchain;
		uint32_t _swapchainWidth;
		uint32_t _swapchainHeight;
		std::vector<VkImage> _swapchainImages;
		std::vector<VkImageView> _swapchainImageViews;
		VkSurfaceFormatKHR _swapchainFormat;
		VkPresentModeKHR _presentMode;

		uint32_t _graphicsQueueFamilyIndex;
		uint32_t _presentQueueFamilyIndex;
		uint32_t _transferQueueFamilyIndex;
		uint32_t _computeQueueFamilyIndex;

		VkQueue _graphicsQeueu;
		VkQueue _presentQueue;
		VkQueue _transferQeueu;
		VkQueue _computeQueue;

		std::unordered_map<VkQueueFlagBits, uint32_t> queueFamilyIndices;
		std::unordered_map<VkQueueFlagBits, VkQueue> queues;

		VmaAllocator _gpuAllocator;
		VmaAllocator _generalPurposeAllocator;
		VmaAllocator _textureAllocator;
		VmaAllocator _meshAllocator;

		VkSemaphore _presentSemaphore;
		uint32_t _currentImageIndex = 0;
		
	private:
		VkResult createInstance(std::vector<const char*>& requiredExtensions, std::vector<const char*>& rquiredValidationLayers, uint32_t versionMajor, uint32_t versionMinor, uint32_t versionPatch);
		VkResult checkInstanceExtensions(std::vector<const char*>& requiredExtensions);
		VkResult checkInstanceValidationLayers(std::vector<const char*>& rquiredValidationLayers);
		

		VkResult createDebugMessenger(VkDebugUtilsMessageSeverityFlagsEXT severityFlags, VkDebugUtilsMessageTypeFlagsEXT typeFlags);
		static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity, VkDebugUtilsMessageTypeFlagsEXT messageType,
			const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData, void* pUserData);


		VkResult pickPhysicalDevice(std::vector<const char*> requiredDeviceExtensions, std::vector<VkQueueFlagBits> desiredQueues, uint32_t requiredAPIVerison);
		VkResult checkPQueueFamilies(std::vector<VkQueueFlagBits> desiredQueues, VkPhysicalDevice physicalDevice);
		VkResult checkDeviceExtenions(std::vector<const char*> requiredDeviceExtensions, VkPhysicalDevice physicalDevice);

		void getQueueFamilyIndices(std::vector<VkQueueFlagBits> desiredQueues, VkPhysicalDevice physicalDevice, std::multimap<VkQueueFlagBits, uint32_t>& indicesMap);
		VkResult getQueueFamilyIndex(VkPhysicalDevice physicalDevice, VkQueueFlagBits desiredQueue, uint32_t& QueueFamilyIndex);
		void getPresentQueueFamilyIndices(VkPhysicalDevice physicalDevice, VkSurfaceKHR surface, std::vector<uint32_t>& presentQueueFamilyIndices);
		VkResult createLogicalDevice(std::vector<VkQueueFlagBits> desiredQueues, std::vector<const char*>& requiredDeviceExtensions, std::vector<const char*>& rquiredValidationLayers, const void* pNextChain);
		void getDeviceQueues();

		VkResult createSwapchain(std::vector<VkSurfaceFormatKHR> desiredSurfaceFormats, VkPresentModeKHR desiredPresentMode, VkExtent2D extends, uint32_t swapchainSize);
		VkResult getDesiredSurfaceFormatPresentMode(std::vector<VkSurfaceFormatKHR> desiredSurfaceFormats, VkPresentModeKHR desiredPresentMode);
		VkResult createSwapchainImageViews();
	};
}