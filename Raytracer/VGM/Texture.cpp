#include "Texture.h"
#include "Texture.h"
#include "Texture.h"
#include "Texture.h"
#include "Texture.h"
#include "Texture.h"

VGM::Texture::Texture(VkFormat format, VkImageType type, VkImageUsageFlags usage, VkExtent3D dimensions, uint32_t arrayLayers, uint32_t mipMapLevels, VmaAllocator& allocator)
{
	switch (format)
	{
	case VK_FORMAT_D32_SFLOAT:
		_imageAspect = VK_IMAGE_ASPECT_DEPTH_BIT;
		break;

	case VK_FORMAT_S8_UINT:
		_imageAspect = VK_IMAGE_ASPECT_STENCIL_BIT;
		break;

	case VK_FORMAT_D32_SFLOAT_S8_UINT:
		_imageAspect = VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT;
		break;

	default:
		_imageAspect = VK_IMAGE_ASPECT_COLOR_BIT;
	}

	if (type == VK_IMAGE_TYPE_1D && arrayLayers > 1)
		_viewType = VK_IMAGE_VIEW_TYPE_1D_ARRAY;
	if (type == VK_IMAGE_TYPE_2D && arrayLayers > 1)
		_viewType = VK_IMAGE_VIEW_TYPE_2D_ARRAY;
	if (type == VK_IMAGE_TYPE_3D)
		_viewType = VK_IMAGE_VIEW_TYPE_3D;
	if (type == VK_IMAGE_TYPE_1D && arrayLayers == 1)
		_viewType = VK_IMAGE_VIEW_TYPE_1D;
	if (type == VK_IMAGE_VIEW_TYPE_2D && arrayLayers == 1)
		_viewType = VK_IMAGE_VIEW_TYPE_2D;

	_format = format;
	_samples = VK_SAMPLE_COUNT_1_BIT;
	_arrayLayers = arrayLayers;
	_mipMapLevels = mipMapLevels;
	_extends = dimensions;

	VkImageCreateInfo ici = {};
	ici.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
	ici.pNext = nullptr;
	ici.arrayLayers = arrayLayers;
	ici.extent = dimensions;
	ici.flags = 0;
	ici.format = format;
	ici.imageType = type;
	ici.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	ici.mipLevels = mipMapLevels;
	ici.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
	ici.samples = VK_SAMPLE_COUNT_1_BIT;
	ici.tiling = VK_IMAGE_TILING_OPTIMAL;
	ici.usage = usage;

	VmaAllocationCreateInfo aci = {};
	aci.usage = VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE;

	vmaCreateImage(allocator, &ici, &aci, &_image, &_allocation, nullptr);
}

VkImage VGM::Texture::get()
{
	return _image;
}

void VGM::Texture::createImageView(uint32_t mipOffset, uint32_t mipMapLevelCount, uint32_t arrayOffset, uint32_t arrayLayersCount, VkDevice device, VkImageView* view)
{
	VkImageSubresourceRange subresource;
	subresource.aspectMask = _imageAspect;
	subresource.baseArrayLayer = arrayOffset;
	subresource.baseMipLevel = mipOffset;
	subresource.layerCount = arrayLayersCount;
	subresource.levelCount = mipMapLevelCount;

	VkImageViewCreateInfo vci = {};
	vci.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
	vci.pNext = nullptr;
	vci.components = { VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY ,VK_COMPONENT_SWIZZLE_IDENTITY };
	vci.format = _format;
	vci.image = _image;
	vci.viewType = _viewType;
	vci.subresourceRange = subresource;

	vkCreateImageView(device, &vci, nullptr, view);
}

VkImageMemoryBarrier2 VGM::Texture::createImageMemoryBarrier2(VkImageSubresourceRange subresource, VkImageLayout oldLayout, VkImageLayout newLayout, 
	VkAccessFlags2 srcAccessFlags, VkAccessFlags2 dstAccessFlags, VkPipelineStageFlags2 srcStageFlags, VkPipelineStageFlags2 dstStageFlags, 
	uint32_t srcIndex, uint32_t dstIndex)
{
	VkImageMemoryBarrier2 barrier = {};
	barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2;
	barrier.pNext = nullptr;
	barrier.image = _image;
	barrier.subresourceRange = subresource;
	barrier.srcQueueFamilyIndex = srcIndex;
	barrier.dstQueueFamilyIndex = dstIndex;
	barrier.oldLayout = oldLayout;
	barrier.newLayout = newLayout;
	barrier.srcAccessMask = srcAccessFlags;
	barrier.dstAccessMask = dstAccessFlags;
	barrier.srcStageMask = srcStageFlags;
	barrier.dstStageMask = dstStageFlags;

	return barrier;
}

void VGM::Texture::cmdUploadTextureData(VkCommandBuffer commandBuffer, uint32_t arrayLayers, uint32_t baseArrayLayer, Buffer& transferBuffer)
{
	VkBufferImageCopy bic = {};
	bic.bufferImageHeight = 0;
	bic.bufferOffset = 0;
	bic.bufferRowLength = 0;
	bic.imageExtent = _extends;

	bic.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	bic.imageSubresource.baseArrayLayer = baseArrayLayer;
	bic.imageSubresource.layerCount = arrayLayers;
	bic.imageSubresource.mipLevel = 0;

	VkImageSubresourceRange subresource;
	subresource.aspectMask = _imageAspect;
	subresource.baseArrayLayer = baseArrayLayer;
	subresource.baseMipLevel = 0;
	subresource.layerCount = arrayLayers;
	subresource.levelCount = _mipMapLevels;

	cmdTransitionLayout(commandBuffer, subresource, VK_IMAGE_LAYOUT_UNDEFINED,
		VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
		VK_ACCESS_NONE,
		VK_ACCESS_TRANSFER_WRITE_BIT,
		VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT);

	vkCmdCopyBufferToImage(commandBuffer, transferBuffer.get(), _image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &bic);

	if (_mipMapLevels > 1)
	{
		cmdGenerateMipMaps(commandBuffer);
	}
	else
	{
		cmdTransitionLayout(commandBuffer, subresource, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
			VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
			VK_ACCESS_NONE,
			VK_ACCESS_SHADER_READ_BIT,
			VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT);
	}
}

void VGM::Texture::cmdGenerateMipMaps(VkCommandBuffer& commandBuffer)
{
	uint32_t mipWidth = _extends.width;
	uint32_t mipHeight = _extends.height;


	for (uint32_t i = 1; i < _mipMapLevels; i++)
	{
		VkOffset3D srcOffset;
		srcOffset.x = mipWidth;
		srcOffset.y = mipHeight;
		srcOffset.z = 1;

		VkOffset3D dstOffset;
		dstOffset.x = mipWidth > 1 ? mipWidth / 2 : 1;
		dstOffset.y = mipHeight > 1 ? mipHeight / 2 : 1;
		dstOffset.z = 1;

		VkImageSubresourceLayers srcSubresource;
		srcSubresource.aspectMask = _imageAspect;
		srcSubresource.baseArrayLayer = 0;
		srcSubresource.layerCount = _arrayLayers;
		srcSubresource.mipLevel = i - 1;

		VkImageSubresourceLayers dstSubresource;
		dstSubresource.aspectMask = _imageAspect;
		dstSubresource.baseArrayLayer = 0;
		dstSubresource.layerCount = _arrayLayers;
		dstSubresource.mipLevel = i;

		VkImageSubresourceRange subresource;
		subresource.aspectMask = _imageAspect;
		subresource.baseArrayLayer = 0;
		subresource.baseMipLevel = i - 1;
		subresource.layerCount = _arrayLayers;
		subresource.levelCount = 1;

		cmdTransitionLayout(commandBuffer, subresource, VK_IMAGE_LAYOUT_UNDEFINED,
			VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
			VK_ACCESS_TRANSFER_WRITE_BIT,
			VK_ACCESS_TRANSFER_READ_BIT,
			VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT);

		cmdBlit(commandBuffer, *this, dstOffset, srcOffset, srcSubresource, dstSubresource, VK_FILTER_LINEAR);

		cmdTransitionLayout(commandBuffer, subresource, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
			VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
			VK_ACCESS_TRANSFER_READ_BIT,
			VK_ACCESS_SHADER_READ_BIT,
			VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT);

		if (mipWidth > 1) mipWidth /= 2;
		if (mipHeight > 1)mipHeight /= 2;
	}

	VkImageSubresourceRange subresource;
	subresource.aspectMask = _imageAspect;
	subresource.baseArrayLayer = 0;
	subresource.baseMipLevel = _mipMapLevels-1;
	subresource.layerCount = _arrayLayers;
	subresource.levelCount = 1;

	cmdTransitionLayout(commandBuffer, subresource, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
		VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
		VK_ACCESS_TRANSFER_WRITE_BIT,
		VK_ACCESS_SHADER_READ_BIT,
		VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT);
}

void VGM::Texture::cmdBlit(VkCommandBuffer& commandBuffer, Texture& srcTexture, VkOffset3D dstOffset, VkOffset3D srcOffset, VkImageSubresourceLayers srcSubresource, VkImageSubresourceLayers dstSubresource, VkFilter filter)
{
	VkImageBlit ib = {};
	ib.dstOffsets[0] = { 0, 0, 0 };
	ib.dstOffsets[1] = dstOffset;
	ib.srcOffsets[0] = { 0, 0, 0 };
	ib.srcOffsets[1] = srcOffset;
	ib.dstSubresource = dstSubresource;
	ib.srcSubresource = srcSubresource;

	vkCmdBlitImage(commandBuffer, srcTexture.get(), VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, _image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &ib, filter);
}

void VGM::Texture::cmdTransitionLayout(VkCommandBuffer commandBuffer, VkImageSubresourceRange subresource, VkImageLayout oldLayout, VkImageLayout newLayout, VkAccessFlags srcAccessFlags, VkAccessFlags dstAccessFlags, VkPipelineStageFlags srcStageFlags, VkPipelineStageFlags dstStageFlags)
{
	VkImageMemoryBarrier imb = {};
	imb.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
	imb.pNext = nullptr;
	imb.srcAccessMask = srcAccessFlags;
	imb.dstAccessMask = dstAccessFlags;
	imb.image = _image;
	imb.newLayout = newLayout;
	imb.oldLayout = oldLayout;
	imb.subresourceRange = subresource;

	vkCmdPipelineBarrier(commandBuffer, srcStageFlags, dstStageFlags, 0, 0, nullptr, 0, nullptr, 1, &imb);
}

void VGM::Texture::cmdPrepareTextureForFragmentShaderWrite(VkCommandBuffer commandBuffer)
{
	VkImageSubresourceRange subresource = {};
	subresource.aspectMask = _imageAspect;
	subresource.baseArrayLayer = 0;
	subresource.baseMipLevel = 0;
	subresource.layerCount = _arrayLayers;
	subresource.levelCount = 1;

	VkAccessFlags access = VK_ACCESS_NONE;
	VkPipelineStageFlagBits pipelineStage = VK_PIPELINE_STAGE_NONE;
	VkImageLayout layout;
	if (_imageAspect == VK_IMAGE_ASPECT_COLOR_BIT)
	{
		access = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
		pipelineStage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
		layout = VK_IMAGE_LAYOUT_ATTACHMENT_OPTIMAL;
	}
	if (_imageAspect == VK_IMAGE_ASPECT_DEPTH_BIT || _imageAspect == VK_IMAGE_ASPECT_STENCIL_BIT)
	{
		access = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT;
		pipelineStage = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
		layout = VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL;
	}

	cmdTransitionLayout(commandBuffer, subresource,
		VK_IMAGE_LAYOUT_UNDEFINED,
		layout, VK_ACCESS_NONE,
		access,
		VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, pipelineStage);
}

void VGM::Texture::cmdPrepareTextureForFragmentShaderRead(VkCommandBuffer commandBuffer)
{
	VkImageSubresourceRange subresource = {};
	subresource.aspectMask = _imageAspect;
	subresource.baseArrayLayer = 0;
	subresource.baseMipLevel = 0;
	subresource.layerCount = _arrayLayers;
	subresource.levelCount = 1;

	VkAccessFlags access = VK_ACCESS_NONE;
	VkPipelineStageFlagBits pipelineStage = VK_PIPELINE_STAGE_NONE;
	VkImageLayout layout;
	if (_imageAspect == VK_IMAGE_ASPECT_COLOR_BIT)
	{
		access = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
		pipelineStage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
		layout = VK_IMAGE_LAYOUT_ATTACHMENT_OPTIMAL;
	}
	if (_imageAspect == VK_IMAGE_ASPECT_DEPTH_BIT || _imageAspect == VK_IMAGE_ASPECT_STENCIL_BIT)
	{
		access = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT;
		pipelineStage = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
		layout = VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL;
	}

	cmdTransitionLayout(commandBuffer, subresource, layout,
		VK_IMAGE_LAYOUT_READ_ONLY_OPTIMAL, access,
		VK_ACCESS_SHADER_READ_BIT,
		pipelineStage, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT);
}

VkImageViewType VGM::Texture::viewType()
{
	return _viewType;
}

VkImageAspectFlags VGM::Texture::imageAspect()
{
	return _imageAspect;
}

VkSampleCountFlagBits VGM::Texture::samples()
{
	return _samples;
}

VkExtent3D VGM::Texture::extends()
{
	return _extends;
}

VkFormat VGM::Texture::format()
{
	return _format;
}

uint32_t VGM::Texture::arrayLayers()
{
	return _arrayLayers;
}

uint32_t VGM::Texture::mipMapLevels()
{
	return _mipMapLevels;
}

void VGM::Texture::destroy(VmaAllocator& allocator)
{
	vmaDestroyImage(allocator, _image, _allocation);
}
