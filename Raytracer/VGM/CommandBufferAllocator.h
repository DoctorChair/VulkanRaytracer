#pragma once
#include "vulkan/vulkan.h"
#include <vector>

namespace VGM
{
	class CommandBufferAllocator
	{
	public:
		CommandBufferAllocator() = default;
		CommandBufferAllocator(uint32_t queueFamilyIndex, VkDevice device);
		VkResult allocateCommandBuffer(VkCommandBuffer* commandBuffer, VkCommandBufferLevel level, VkDevice device);
		VkResult allocateCommandBuffers(std::vector<VkCommandBuffer>& commandBuffers, VkCommandBufferLevel level, VkDevice device);
		VkResult reset(VkDevice device);
		void destroy(VkDevice device);

	private:
		VkCommandPool _commandPool;
	};
}