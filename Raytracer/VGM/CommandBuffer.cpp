#include "CommandBuffer.h"

VGM::CommandBuffer::CommandBuffer(CommandBufferAllocator& allocator, VkDevice device)
{
	allocator.allocateCommandBuffer(&_commandBuffer, VK_COMMAND_BUFFER_LEVEL_PRIMARY, device);
}

VkResult VGM::CommandBuffer::begin(VkFence* pWaitFences, uint32_t waitFenceCount, VkDevice device)
{
	if (waitFenceCount != 0)
	{
		vkWaitForFences(device, waitFenceCount, pWaitFences, VK_TRUE, UINT64_MAX);
		vkResetFences(device, waitFenceCount, pWaitFences);
	}
	vkResetCommandBuffer(_commandBuffer, 0);

	VkCommandBufferBeginInfo beginInfo = {};
	beginInfo.pNext = nullptr;
	beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
	beginInfo.pInheritanceInfo = nullptr;

	return vkBeginCommandBuffer(_commandBuffer, &beginInfo);
}

VkCommandBuffer VGM::CommandBuffer::get()
{
	return _commandBuffer;
}

VkResult VGM::CommandBuffer::end()
{
	return vkEndCommandBuffer(_commandBuffer);
}

VkResult VGM::CommandBuffer::submit(VkSemaphore* pSignalSemaphores, uint32_t signalSemaphoreCount, VkSemaphore* pWaitSemaphores, uint32_t waitSemaphoreCount, VkPipelineStageFlags* pWaitStageFlags, VkFence signalFence, VkQueue& queue)
{
	VkSubmitInfo submitInfo = {};
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submitInfo.pNext = nullptr;
	submitInfo.pSignalSemaphores = pSignalSemaphores;
	submitInfo.signalSemaphoreCount = signalSemaphoreCount;
	submitInfo.pWaitSemaphores = pWaitSemaphores;
	submitInfo.waitSemaphoreCount = waitSemaphoreCount;
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &_commandBuffer;
	submitInfo.pWaitDstStageMask = pWaitStageFlags;

	return vkQueueSubmit(queue, 1, &submitInfo, signalFence);
}
