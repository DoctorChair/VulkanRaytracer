#include "Image.h"

Image::Image(VkFormat format, VkImageType type, VkImageUsageFlags usage, VkSampleCountFlagBits sampleCount, uint32_t width, uint32_t height, uint32_t depth, uint32_t arrayLayers, uint32_t mipMapLevels, VkDevice& device, VmaAllocator& allocator)
{
	VmaAllocationCreateInfo allocInfo = {};
	allocInfo.usage = VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE;
	allocInfo.flags = VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT;

	VkImageCreateInfo imageInfo = {};
	imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
	imageInfo.arrayLayers = arrayLayers;
	imageInfo.mipLevels = mipMapLevels;
	imageInfo.format = format;
	imageInfo.imageType = type;
	imageInfo.extent = { width, height, depth };
	imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	imageInfo.samples = sampleCount;
	imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
	imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
	imageInfo.usage = usage;
	imageInfo.flags = 0;

	vmaCreateImage(allocator, &imageInfo, &allocInfo, &_image, &_allocation, nullptr);

	_format = format;
	_width = width;
	_height = height;
	_depth = depth;
	_sampleCount = sampleCount;
	_arrayLayers = arrayLayers;
	_mipMapLevels = mipMapLevels;
}

void Image::destroy(VmaAllocator& allocator)
{
	vmaDestroyImage(allocator, _image, _allocation);
}

void Image::createImageView(VkFormat format, VkImageSubresourceRange subresourceRange, VkImageViewType type, VkComponentMapping componentMapping, VkImageView* view, VkDevice& device)
{
	VkImageViewCreateInfo info = {};
	info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
	info.image = _image;
	info.flags = 0;
	info.format = format;
	info.subresourceRange = subresourceRange;
	info.viewType = type;
	info.components = componentMapping;
	vkCreateImageView(device, &info, nullptr, view);
}
