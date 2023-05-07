#pragma once
#include <vector>
#include <vulkan/vulkan.h>
#include "Texture.h"

namespace VGM
{

	class Framebuffer
	{
	public:
		Framebuffer() = default;
		void cmdBeginRendering(VkCommandBuffer commandBuffer, VkRect2D renderArea);
		void cmdEndRendering(VkCommandBuffer commandBuffer);
		void bindColorTextureTarget(VkImageView& view, VkAttachmentLoadOp loadOp, VkAttachmentStoreOp storeOp, VkClearColorValue clearColor);
		void bindDepthTextureTarget(VkImageView& view, VkAttachmentLoadOp loadOp, VkAttachmentStoreOp storeOp, float clearValue);
		void bindStencilTextureTarget(VkImageView& view, VkAttachmentLoadOp loadOp, VkAttachmentStoreOp storeOp, float clearValue);
		void resetBindings();

	private:
		VkRenderingAttachmentInfo setAttachmentInfo(VkImageView view, VkImageLayout layout, VkAttachmentLoadOp loadOp, VkAttachmentStoreOp storeOp, VkClearValue clearValue);

		std::vector<VkImageView> _imageViews;
		std::vector<VkRenderingAttachmentInfo> _colorAttachments;
		VkRenderingAttachmentInfo _depthAttachment;
		VkRenderingAttachmentInfo* _pDepthAttachment = nullptr;
		VkRenderingAttachmentInfo _stencilAttachment;
		VkRenderingAttachmentInfo* _pStencilAttachment = nullptr;
	};

}