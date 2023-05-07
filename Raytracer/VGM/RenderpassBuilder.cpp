#include "RenderpassBuilder.h"

VGM::RenderpassBuilder::RenderpassBuilder(uint32_t subpassCount)
{
	_colorAttachmentReferences.resize(subpassCount);
	_inputAttachmentReferences.resize(subpassCount);
	_depthStencilAttachmentReferences.resize(subpassCount);
	_subpasses.resize(subpassCount);
}

VGM::RenderpassBuilder& VGM::RenderpassBuilder::beginSubpassDescription()
{
	return *this;
}

VGM::RenderpassBuilder& VGM::RenderpassBuilder::endSubpassDescription()
{
	_subpasses[_currentSubpass].pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
	_subpasses[_currentSubpass].pColorAttachments = _colorAttachmentReferences.back().data();
	_subpasses[_currentSubpass].colorAttachmentCount = _colorAttachmentReferences.back().size();
	_subpasses[_currentSubpass].preserveAttachmentCount = 0;
	_subpasses[_currentSubpass].pPreserveAttachments = nullptr;
	_subpasses[_currentSubpass].pResolveAttachments = nullptr;
	_subpasses[_currentSubpass].pInputAttachments = _inputAttachmentReferences.back().data();
	_subpasses[_currentSubpass].inputAttachmentCount = _inputAttachmentReferences.back().size();
	_subpasses[_currentSubpass].pDepthStencilAttachment = (_hasDepth) ? &_depthStencilAttachmentReferences[_currentSubpass] : nullptr;

	_currentSubpass++;
	_hasDepth = false;
	return *this;
}

VGM::RenderpassBuilder& VGM::RenderpassBuilder::bindDepthStencilImage(VkFormat format)
{
	VkAttachmentDescription attachmentDescription = {};
	attachmentDescription.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	attachmentDescription.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
	attachmentDescription.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	attachmentDescription.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	attachmentDescription.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	attachmentDescription.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
	attachmentDescription.format = format;
	attachmentDescription.samples = VK_SAMPLE_COUNT_1_BIT;

	_attachmentDescriptions.push_back(attachmentDescription);

	VkAttachmentReference attachmentReference = {};
	attachmentReference.attachment = _attachmentDescriptions.size()-1;
	attachmentReference.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

	_depthStencilAttachmentReferences.push_back(attachmentReference);

	VkSubpassDependency dependency = {};
	dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
	dependency.dstSubpass = _currentSubpass;
	dependency.srcStageMask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
	dependency.dstStageMask = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
	dependency.srcAccessMask = VK_ACCESS_NONE;
	dependency.dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
	
	_subpassDependencies.push_back(dependency);
	_hasDepth = true;

	return *this;
}

VGM::RenderpassBuilder& VGM::RenderpassBuilder::bindOutputImage(VkFormat format)
{
	VkAttachmentDescription attachmentDescription = {};
	attachmentDescription.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	attachmentDescription.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
	attachmentDescription.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	attachmentDescription.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	attachmentDescription.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	attachmentDescription.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
	attachmentDescription.format = format;
	attachmentDescription.samples = VK_SAMPLE_COUNT_1_BIT;

	_attachmentDescriptions.push_back(attachmentDescription);

	VkAttachmentReference attachmentReference = {};
	attachmentReference.attachment = _attachmentDescriptions.size() - 1;
	attachmentReference.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

	_colorAttachmentReferences[_currentSubpass].push_back(attachmentReference);

	VkSubpassDependency dependency = {};
	dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
	dependency.dstSubpass = _currentSubpass;
	dependency.srcStageMask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
	dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	dependency.srcAccessMask = VK_ACCESS_NONE;
	dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

	_subpassDependencies.push_back(dependency);

	return *this;
}


void VGM::RenderpassBuilder::build(VkDevice device, VkRenderPass* renderpass)
{
	VkRenderPassCreateInfo renderpassInfo = {};
	renderpassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
	renderpassInfo.pSubpasses = _subpasses.data();
	renderpassInfo.subpassCount = _subpasses.size();
	renderpassInfo.pDependencies = _subpassDependencies.data();
	renderpassInfo.dependencyCount = _subpassDependencies.size();
	renderpassInfo.pAttachments = _attachmentDescriptions.data();
	renderpassInfo.attachmentCount = _attachmentDescriptions.size();
	
	vkCreateRenderPass(device, &renderpassInfo, nullptr, renderpass);
}
