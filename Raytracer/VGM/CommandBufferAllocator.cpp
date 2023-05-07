#include "CommandBufferAllocator.h"
#include <iostream>

VGM::CommandBufferAllocator::CommandBufferAllocator(uint32_t queueFamilyIndex, VkDevice device)
{
	VkCommandPoolCreateInfo createInfo = {};
	createInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
	createInfo.pNext = nullptr;
	createInfo.queueFamilyIndex = queueFamilyIndex;
	createInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;

	vkCreateCommandPool(device, &createInfo, nullptr, &_commandPool);
}

VkResult VGM::CommandBufferAllocator::allocateCommandBuffer(VkCommandBuffer* commandBuffer, VkCommandBufferLevel level, VkDevice device)
{
	VkCommandBufferAllocateInfo allocInfo = {};
	allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	allocInfo.commandBufferCount = 1;
	allocInfo.commandPool = _commandPool;
	allocInfo.level = level;
	allocInfo.pNext = nullptr;

	return vkAllocateCommandBuffers(device, &allocInfo, commandBuffer);
}

VkResult VGM::CommandBufferAllocator::allocateCommandBuffers(std::vector<VkCommandBuffer>& commandBuffers, VkCommandBufferLevel level, VkDevice device)
{
	VkCommandBufferAllocateInfo allocInfo = {};
	allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	allocInfo.commandBufferCount = commandBuffers.size();
	allocInfo.commandPool = _commandPool;
	allocInfo.level = level;
	allocInfo.pNext = nullptr;

	return vkAllocateCommandBuffers(device, &allocInfo, commandBuffers.data());
}

VkResult VGM::CommandBufferAllocator::reset(VkDevice device)
{
	return vkResetCommandPool(device, _commandPool, 0);
}

void VGM::CommandBufferAllocator::destroy(VkDevice device)
{
	vkDestroyCommandPool(device, _commandPool, nullptr);
}
