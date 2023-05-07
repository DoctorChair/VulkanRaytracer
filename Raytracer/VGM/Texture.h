#pragma once
#include <vulkan/vulkan.h>
#include <vma/vk_mem_alloc.h>
#include "Buffer.h"

namespace VGM
{
	class Texture
	{
	public:
		Texture() = default;
		Texture(VkFormat format, VkImageType type, VkImageUsageFlags usage, VkExtent3D dimensions, uint32_t arrayLayers, uint32_t mipMapLevels, VmaAllocator& allocator);
		VkImage get();
		void createImageView(uint32_t mipOffset, uint32_t mipMapLevelCount, uint32_t arrayOffset, uint32_t arrayLayersCount, VkDevice device, VkImageView* view);
		VkImageMemoryBarrier2 createImageMemoryBarrier2(VkImageSubresourceRange subresource, VkImageLayout oldLayout, VkImageLayout newLayout,
			VkAccessFlags2 srcAccessFlags, VkAccessFlags2 dstAccessFlags,
			VkPipelineStageFlags2 srcStageFlags, VkPipelineStageFlags2 dstStageFlags, uint32_t srcIndex, uint32_t dstIndex);

		void cmdUploadTextureData(VkCommandBuffer commandBuffer, uint32_t arrayLayers, uint32_t baseArrayLayer, Buffer& transferBuffer);
		void cmdTransitionLayout(VkCommandBuffer commandBuffer, VkImageSubresourceRange subresource, VkImageLayout oldLayout, VkImageLayout newLayout, VkAccessFlags srcAccessFlags, VkAccessFlags dstAccessFlags, VkPipelineStageFlags srcStageFlags, VkPipelineStageFlags dstStageFlags);
		void cmdPrepareTextureForFragmentShaderWrite(VkCommandBuffer commandBuffer);
		void cmdPrepareTextureForFragmentShaderRead(VkCommandBuffer commandBuffer);

		VkImageViewType viewType();
		VkImageAspectFlags imageAspect();
		VkSampleCountFlagBits samples();
		VkExtent3D extends();
		VkFormat format();
		uint32_t arrayLayers();
		uint32_t mipMapLevels();
		
		void destroy(VmaAllocator& allocator);

	private:
		void cmdGenerateMipMaps(VkCommandBuffer& commandBuffer);
		void cmdBlit(VkCommandBuffer& commandBuffer, Texture& srcTexture, VkOffset3D dstOffset, VkOffset3D srcOffset, VkImageSubresourceLayers srcSubresource, VkImageSubresourceLayers dstSubresource, VkFilter filter);

		VkImage _image;
		VmaAllocation _allocation;

		VkFormat _format;
		uint32_t _arrayLayers;
		uint32_t _mipMapLevels;
		VkSampleCountFlagBits _samples;
		VkExtent3D _extends;

		VkImageAspectFlags _imageAspect;
		VkImageViewType _viewType;
	};
};
