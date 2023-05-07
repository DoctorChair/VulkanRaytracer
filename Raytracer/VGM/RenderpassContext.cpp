#include "RenderpassBuilder.h"

graphics::RenderpassBuilder::RenderpassBuilder(uint32_t subpassCount)
{
	_colorAttachmentReferences.resize(subpassCount);
	_inputAttachmentReferences.resize(subpassCount);
	_depthStencilAttachmentReferences.resize(subpassCount);
	_subpasses.resize(subpassCount);
}

graphics::RenderpassBuilder& graphics::RenderpassBuilder::beginSubpassDescription()
{
	return *this;
}

graphics::RenderpassBuilder& graphics::RenderpassBuilder::endSubpassDescription()
{
	_subpasses[_currentSubpass].pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
	_subpasses[_currentSubpass].pColorAttachments = _colorAttachmentReferences.back().data();
	_subpasses[_currentSubpass].colorAttachmentCount = _colorAttachmentReferences.back().size();
	_subpasses[_currentSubpass].preserveAttachmentCount = 0;
	_subpasses[_currentSubpass].pPreserveAttachments = nullptr;
	_subpasses[_currentSubpass].pResolveAttachments = nullptr;
	_subpasses[_currentSubpass].pInputAttachments = _inputAttachmentReferences.back().data();
	_subpasses[_currentSubpass].inputAttachmentCount = _inputAttachmentReferences.back().size();
	_subpasses[_currentSubpass].pDepthStencilAttachment = &_depthStencilAttachmentReferences[_currentSubpass];
	
	_currentSubpass++;

	return *this;
}

graphics::RenderpassBuilder& graphics::RenderpassBuilder::bindDepthImage(VkFormat format)
{
	VkAttachmentDescription attachmentDescription = {};
	attachmentDescription.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	attachmentDescription.finalLayout = VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL;
	attachmentDescription.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	attachmentDescription.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	attachmentDescription.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	attachmentDescription.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
	attachmentDescription.format = format;
	attachmentDescription.samples = VK_SAMPLE_COUNT_1_BIT;

	_attachmentDescriptions.push_back(attachmentDescription);

	VkAttachmentReference attachmentReference = {};
	attachmentReference.attachment = _attachmentDescriptions.size()-1;
	attachmentReference.layout = VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL;

	_depthStencilAttachmentReferences.push_back(attachmentReference);

	uint32_t srcSubpass = (_currentSubpass == 0) ? VK_SUBPASS_EXTERNAL : _currentSubpass - 1;

	VkSubpassDependency dependency = {};
	dependency.srcSubpass = srcSubpass;
	dependency.dstSubpass = _currentSubpass;
	dependency.srcStageMask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
	dependency.dstStageMask = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
	dependency.srcAccessMask = VK_ACCESS_NONE;
	dependency.dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
	
	_subpassDependencies.push_back(dependency);

	return *this;
}

graphics::RenderpassBuilder& graphics::RenderpassBuilder::bindOutputImage(VkFormat format)
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

	uint32_t srcSubpass = (_currentSubpass == 0) ? VK_SUBPASS_EXTERNAL : _currentSubpass - 1;

	VkSubpassDependency dependency = {};
	dependency.srcSubpass = srcSubpass;
	dependency.dstSubpass = _currentSubpass;
	dependency.srcStageMask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
	dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	dependency.srcAccessMask = VK_ACCESS_NONE;
	dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

	_subpassDependencies.push_back(dependency);

	return *this;
}

graphics::RenderpassBuilder& graphics::RenderpassBuilder::bindInputImage(VkFormat format)
{
	VkAttachmentDescription attachmentDescription = {};
	attachmentDescription.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	attachmentDescription.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
	attachmentDescription.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	attachmentDescription.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	attachmentDescription.loadOp = VK_ATTACHMENT_LOAD_OP_LOAD;
	attachmentDescription.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	attachmentDescription.format = format;
	attachmentDescription.samples = VK_SAMPLE_COUNT_1_BIT;

	_attachmentDescriptions.push_back(attachmentDescription);

	VkAttachmentReference attachmentReference = {};
	attachmentReference.attachment = _attachmentDescriptions.size() - 1;
	attachmentReference.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

	_inputAttachmentReferences[_currentSubpass].push_back(attachmentReference);

	uint32_t srcSubpass = (_currentSubpass == 0) ? VK_SUBPASS_EXTERNAL : _currentSubpass - 1;

	VkSubpassDependency dependency = {};
	dependency.srcSubpass = srcSubpass;
	dependency.dstSubpass = _currentSubpass;
	dependency.srcStageMask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
	dependency.dstStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
	dependency.srcAccessMask = VK_ACCESS_NONE;
	dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT;

	_subpassDependencies.push_back(dependency);

	return *this;
}

void graphics::RenderpassBuilder::build(VkRenderPass* renderpas, VkDevice& device)
{
	VkRenderPassCreateInfo renderpassInfo = {};
	renderpassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
	renderpassInfo.pSubpasses = _subpasses.data();
	renderpassInfo.subpassCount = _subpasses.size();
	renderpassInfo.pDependencies = _subpassDependencies.data();
	renderpassInfo.dependencyCount = _subpassDependencies.size();
	renderpassInfo.pAttachments = _attachmentDescriptions.data();
	renderpassInfo.attachmentCount = _attachmentDescriptions.size();

	vkCreateRenderPass(device, &renderpassInfo, nullptr, renderpas);
}
