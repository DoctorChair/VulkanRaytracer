#pragma once

#include <vulkan/vulkan.h>
#include <vector>

namespace VGM
{
	class RenderpassBuilder
	{
	public:
		RenderpassBuilder(uint32_t subpassCount);
		RenderpassBuilder& beginSubpassDescription();
		RenderpassBuilder& endSubpassDescription();
		RenderpassBuilder& bindDepthStencilImage(VkFormat format);
		RenderpassBuilder& bindOutputImage(VkFormat format);
		void build(VkDevice device, VkRenderPass* renderpass);

	private:
		uint32_t _currentSubpass = 0;
		bool _hasDepth = false;

		std::vector<VkAttachmentDescription> _attachmentDescriptions;

		std::vector<std::vector<VkAttachmentReference>> _colorAttachmentReferences;
		std::vector<std::vector<VkAttachmentReference>> _inputAttachmentReferences;
		std::vector<VkAttachmentReference> _depthStencilAttachmentReferences;

		std::vector<VkSubpassDependency> _subpassDependencies;
		std::vector<VkSubpassDescription> _subpasses;

		std::vector<VkImageView> _imageViews;
	};
}