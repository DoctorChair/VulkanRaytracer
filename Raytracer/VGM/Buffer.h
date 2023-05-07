#pragma once
#include <vulkan/vulkan.h>
#include <vma/vk_mem_alloc.h>

namespace VGM
{
	class Buffer
	{
	public:
		Buffer() = default;
		Buffer(VkBufferUsageFlags bufferUsage, VkDeviceSize size, VmaAllocator& allocator, VkDevice device);
		Buffer(VkBufferUsageFlags bufferUsage, VmaAllocationCreateFlags flags, VkDeviceSize size, VmaAllocator& allocator, VkDevice device);
		void uploadData(const void* data, size_t length, VmaAllocator allocator);
		void* map(VmaAllocator allocator);
		void memcopy(void* dst, void* source, size_t length);
		void unmap(VmaAllocator allocator);

		VkBufferMemoryBarrier2 createBufferMemoryBarrier2(VkDeviceSize size, VkDeviceSize offset, VkAccessFlags2 srcAccessFlags, VkAccessFlags2 dstAccessFlags, 
			VkPipelineStageFlags2 srcStageFlags, VkPipelineStageFlags2 dstStageFlags,
			uint32_t srcIndex, uint32_t dstIndex);

		void cmdMemoryBarrier(VkCommandBuffer commandBuffer, VkAccessFlags srcAccessMask, VkAccessFlags dstAccessMask, VkPipelineStageFlags srcStageFlags, VkPipelineStageFlags dstStageFlags, VkDependencyFlags dependencyFlags);
		void cmdCopyBuffer(VkCommandBuffer commandBuffer, VkBuffer srcBuffer, uint32_t regionCount, VkBufferCopy* pRegions);
		void cmdAppendCopyBuffer(VkCommandBuffer commandBuffer, VkBuffer srcBuffer, VkDeviceSize length, VkDeviceSize srcOffset);
		VkDeviceAddress getDeviceAddress(VkDevice device);
		VkDeviceSize getAlignment(VkDevice device);
		VkBuffer get();
		VkDeviceSize capacity();
		VkDeviceSize size();
		
		void destroy(VmaAllocator& allocator);

	private:
		VkBuffer _buffer;
		VmaAllocation _allocation;

		VkMemoryRequirements _memoryRequirements;
		VkDeviceSize _capacity; //size in bytes
		VkDeviceSize _size;
	};
}