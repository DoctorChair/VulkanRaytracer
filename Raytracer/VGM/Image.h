#pragma once

#include <vulkan/vulkan.h>
#include <vma/vk_mem_alloc.h>

class Image
{
public:
	Image(VkFormat format, VkImageType type, VkImageUsageFlags usage, VkSampleCountFlagBits sampleCount, uint32_t width, uint32_t height, uint32_t depth, uint32_t arrayLayers, uint32_t mipMapLevels, VkDevice& device, VmaAllocator& allocator);
	void destroy(VmaAllocator& allocator);
	void createImageView(VkFormat format, VkImageSubresourceRange subresourceRange, VkImageViewType type, VkComponentMapping componentMapping, VkImageView* view, VkDevice& device);
	
	void issueLayoutTransitionCommand(VkCommandBuffer& commandBuffer);
	void issueMemoryTransferCommand(VkBuffer buffer, VkCommandBuffer& commandBuffer);

protected:
	VkImage _image;
	VmaAllocation _allocation;

	VkFormat _format;
	VkSampleCountFlagBits _sampleCount;
	uint32_t _width;
	uint32_t _height;
	uint32_t _depth;
	uint32_t _mipMapLevels;
	uint32_t _arrayLayers;
};