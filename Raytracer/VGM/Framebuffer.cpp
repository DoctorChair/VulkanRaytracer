#include "Framebuffer.h"

void VGM::Framebuffer::cmdBeginRendering(VkCommandBuffer commandBuffer, VkRect2D renderArea)
{
	VkRenderingInfo renderingInfo = {};
	renderingInfo.sType = VK_STRUCTURE_TYPE_RENDERING_INFO;
	renderingInfo.pNext = nullptr;
	renderingInfo.colorAttachmentCount = _colorAttachments.size();
	renderingInfo.pColorAttachments = _colorAttachments.data();
	renderingInfo.pDepthAttachment = _pDepthAttachment;
	renderingInfo.pStencilAttachment = _pStencilAttachment;
	renderingInfo.layerCount = 1;
	renderingInfo.viewMask = 0;
	renderingInfo.renderArea = renderArea;

	vkCmdBeginRendering(commandBuffer, &renderingInfo);
}

void VGM::Framebuffer::cmdEndRendering(VkCommandBuffer commandBuffer)
{
	vkCmdEndRendering(commandBuffer);
}

void VGM::Framebuffer::bindColorTextureTarget(VkImageView& view, VkAttachmentLoadOp loadOp, VkAttachmentStoreOp storeOp, VkClearColorValue clearColor)
{
	VkClearValue value;
	value.color = clearColor;

	VkRenderingAttachmentInfo attachmentInfo = setAttachmentInfo(view, VK_IMAGE_LAYOUT_ATTACHMENT_OPTIMAL, loadOp, storeOp, value);
	_colorAttachments.push_back(attachmentInfo);
}

void VGM::Framebuffer::bindDepthTextureTarget(VkImageView& view, VkAttachmentLoadOp loadOp, VkAttachmentStoreOp storeOp, float clearValue)
{
	VkClearValue value;
	value.depthStencil.depth = clearValue;

	VkRenderingAttachmentInfo attachmentInfo = setAttachmentInfo(view, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL, loadOp, storeOp, value);
	_depthAttachment = attachmentInfo;
	_pDepthAttachment = &_depthAttachment;
}

void VGM::Framebuffer::bindStencilTextureTarget(VkImageView& view, VkAttachmentLoadOp loadOp, VkAttachmentStoreOp storeOp, float clearValue)
{
	VkClearValue value;
	value.depthStencil.stencil = clearValue;

	VkRenderingAttachmentInfo attachmentInfo = setAttachmentInfo(view, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL, loadOp, storeOp, value);
	_stencilAttachment = attachmentInfo;
	_pStencilAttachment = &_stencilAttachment;
}

void VGM::Framebuffer::resetBindings()
{
	_colorAttachments.clear();
	_pDepthAttachment = nullptr;
	_pStencilAttachment = nullptr;
}

VkRenderingAttachmentInfo VGM::Framebuffer::setAttachmentInfo(VkImageView view, VkImageLayout layout, VkAttachmentLoadOp loadOp, VkAttachmentStoreOp storeOp, VkClearValue clearValue)
{
	VkRenderingAttachmentInfo attachmentInfo;
	attachmentInfo.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
	attachmentInfo.pNext = nullptr;
	attachmentInfo.imageLayout = layout;
	attachmentInfo.imageView = view;
	attachmentInfo.loadOp = loadOp;
	attachmentInfo.storeOp = storeOp;
	attachmentInfo.clearValue = clearValue;
	attachmentInfo.resolveMode = VK_RESOLVE_MODE_NONE;
	attachmentInfo.resolveImageView = VK_NULL_HANDLE;
	attachmentInfo.resolveImageLayout = VK_IMAGE_LAYOUT_UNDEFINED;

	return attachmentInfo;
}
