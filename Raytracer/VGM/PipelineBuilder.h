#pragma once 
#include <vulkan/vulkan.h>
#include <vector>

namespace VGM
{
	class PipelineConfigurator
	{
	public:
		PipelineConfigurator();
		PipelineConfigurator& addVertexInputInputBindingDescription(uint32_t binding, uint32_t stride, VkVertexInputRate vertrexInputRate);
		PipelineConfigurator& addVertexAttribInputDescription(uint32_t location, uint32_t binding, VkFormat format, uint32_t offset);
		PipelineConfigurator& setViewportState(uint32_t width, uint32_t height, float mindDepth, float maxDepth, float xPos, float yPos);
		PipelineConfigurator& setScissorState(uint32_t width, uint32_t height, int32_t xOffset, int32_t yOffset);
		PipelineConfigurator& setInputAssemblyState(VkPrimitiveTopology primitiv);
		PipelineConfigurator& setRasterizerState(VkPolygonMode mode, VkFrontFace frontFace);
		PipelineConfigurator& setBackfaceCulling(VkCullModeFlags cullMode);
		PipelineConfigurator& setDepthBias(VkBool32 depthBiasEnable, float depthBiasConstantFactor, float depthBiasSlopeFactor, float depthBiasClamp);
		PipelineConfigurator& setMultisamplingState();
		PipelineConfigurator& setDepthState(VkBool32 depthTestEnable, VkBool32 depthWriteEnable, VkCompareOp depthCompareOp);
		PipelineConfigurator& setStencilState(VkBool32 stencilTestEnable, VkStencilOpState frontOp, VkStencilOpState backOp);
		PipelineConfigurator& setColorBlendingState(std::vector<VkPipelineColorBlendAttachmentState>& colorBlendAttachments);
		PipelineConfigurator& addDynamicState(VkDynamicState state);
		PipelineConfigurator& setRenderingFormats(std::vector<VkFormat>& colorFormats, VkFormat depthFormat, VkFormat stencilFormat);
		VkGraphicsPipelineCreateInfo getPipelineConfiguration();

	private:
		std::vector<VkVertexInputBindingDescription> _vertexBindingDescriptions;
		std::vector<VkVertexInputAttributeDescription> _vertexAttributeDescriptons;
		std::vector<VkPipelineColorBlendAttachmentState> _colorBlendAttachments;
		std::vector<VkDynamicState> _dynamicStates;
		std::vector<VkFormat> _colorFormats;

		VkViewport _viewPort;
		VkRect2D _scissor;
		VkPipelineViewportStateCreateInfo _viewportState;
		VkPipelineVertexInputStateCreateInfo _vertexInputInfo;
		VkPipelineInputAssemblyStateCreateInfo _inputAssembly;
		VkPipelineRasterizationStateCreateInfo _rasterizationState;
		VkPipelineMultisampleStateCreateInfo _multisampleState;
		VkPipelineDepthStencilStateCreateInfo _depthStencilState;
		VkPipelineColorBlendStateCreateInfo _colorBlendingState;
		VkPipelineTessellationStateCreateInfo _tesselationState;
		VkPipelineDynamicStateCreateInfo _dynamicState;
		VkPipelineRenderingCreateInfo _renderingState;
	};
}