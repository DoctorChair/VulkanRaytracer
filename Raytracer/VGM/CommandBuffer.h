#pragma once
#include "vulkan/vulkan.h"
#include <vector>
#include "CommandBufferAllocator.h"

namespace VGM
{
	typedef void(*commandRecordFunction)(VkCommandBuffer);

	class CommandBuffer
	{
	public:
		CommandBuffer() = default;
		CommandBuffer(CommandBufferAllocator& allocator, VkDevice device);
		VkResult begin(VkFence* pWaitFences, uint32_t waitFenceCount, VkDevice device);
		VkCommandBuffer get();
		VkResult end();
		VkResult submit(VkSemaphore* pSignalSemaphores, uint32_t signalSemaphoreCount, VkSemaphore* pWaitSemaphores, uint32_t waitSemaphoreCount, VkPipelineStageFlags* pWaitStageFlags, VkFence signalFence, VkQueue& queue);

	private:
		VkCommandBuffer _commandBuffer;

	};
}