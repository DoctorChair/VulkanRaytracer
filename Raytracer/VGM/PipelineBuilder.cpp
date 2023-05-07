#include "PipelineBuilder.h"

VGM::PipelineConfigurator::PipelineConfigurator()
{
	_viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
	_viewportState.pNext = nullptr;
	_viewportState.scissorCount = 0;
	_viewportState.viewportCount = 0;
	_viewportState.flags = 0;

	_vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
	_vertexInputInfo.pNext = nullptr;
	_vertexInputInfo.vertexAttributeDescriptionCount = 0;
	_vertexInputInfo.vertexBindingDescriptionCount = 0;
	_vertexInputInfo.flags = 0;

	_inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
	_inputAssembly.primitiveRestartEnable = VK_FALSE;
	_inputAssembly.pNext = nullptr;
	_inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
	_inputAssembly.flags = 0;

	_rasterizationState.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
	_rasterizationState.pNext = nullptr;
	_rasterizationState.cullMode = VK_CULL_MODE_NONE;
	_rasterizationState.depthBiasClamp = 0.0f;
	_rasterizationState.depthBiasConstantFactor = 0.0f;
	_rasterizationState.depthBiasSlopeFactor = 0.0f;
	_rasterizationState.depthBiasEnable = VK_FALSE;
	_rasterizationState.depthClampEnable = VK_FALSE;
	_rasterizationState.frontFace = VK_FRONT_FACE_CLOCKWISE;
	_rasterizationState.lineWidth = 1.0f;
	_rasterizationState.polygonMode = VK_POLYGON_MODE_FILL;
	_rasterizationState.rasterizerDiscardEnable = VK_FALSE;
	_rasterizationState.flags = 0;

	_multisampleState.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
	_multisampleState.pNext = nullptr;
	_multisampleState.alphaToCoverageEnable = VK_FALSE;
	_multisampleState.alphaToOneEnable = VK_FALSE;
	_multisampleState.minSampleShading = 1.0f;
	_multisampleState.pSampleMask = nullptr;
	_multisampleState.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
	_multisampleState.sampleShadingEnable = VK_FALSE;
	_multisampleState.flags = 0;

	_depthStencilState.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
	_depthStencilState.pNext = nullptr;
	_depthStencilState.depthTestEnable = VK_FALSE;
	_depthStencilState.depthWriteEnable = VK_FALSE;
	_depthStencilState.stencilTestEnable = VK_FALSE;
	_depthStencilState.depthBoundsTestEnable = VK_FALSE;
	_depthStencilState.minDepthBounds = 0.0f;
	_depthStencilState.maxDepthBounds = 1.0f;
	_depthStencilState.flags = 0;

	VkPipelineColorBlendAttachmentState blendAttachment = {};
	blendAttachment.blendEnable = VK_FALSE;
	blendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT
		| VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;

	_colorBlendAttachments.push_back(blendAttachment);

	_colorBlendingState.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
	_colorBlendingState.pNext = nullptr;
	_colorBlendingState.pAttachments = _colorBlendAttachments.data();
	_colorBlendingState.attachmentCount = _colorBlendAttachments.size();
	_colorBlendingState.logicOpEnable = VK_FALSE;
	_colorBlendingState.logicOp = VK_LOGIC_OP_COPY;
	_colorBlendingState.flags = 0;

	_tesselationState.sType = VK_STRUCTURE_TYPE_PIPELINE_TESSELLATION_STATE_CREATE_INFO;
	_tesselationState.pNext = nullptr;
	_tesselationState.patchControlPoints = 3;
	_tesselationState.flags = 0;

	_dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
	_dynamicState.pNext = nullptr;
	_dynamicState.dynamicStateCount = 0;
	_dynamicState.flags = 0;
	
	_renderingState.sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO;
	_renderingState.pNext = nullptr;
}

VGM::PipelineConfigurator& VGM::PipelineConfigurator::addVertexInputInputBindingDescription(uint32_t binding, uint32_t stride, VkVertexInputRate vertrexInputRate)
{
	_vertexBindingDescriptions.push_back({ binding, stride, vertrexInputRate });

	return *this;
}

VGM::PipelineConfigurator& VGM::PipelineConfigurator::addVertexAttribInputDescription(uint32_t location, uint32_t binding, VkFormat format, uint32_t offset)
{
	_vertexAttributeDescriptons.push_back({ location, binding, format, offset });

	return *this;
}

VGM::PipelineConfigurator& VGM::PipelineConfigurator::setViewportState(uint32_t width, uint32_t height, float mindDepth, float maxDepth, float xPos, float yPos)
{
	_viewPort.width = width;
	_viewPort.height = height;
	_viewPort.minDepth = mindDepth;
	_viewPort.maxDepth = maxDepth;
	_viewPort.x = xPos;
	_viewPort.y = yPos;

	_viewportState.viewportCount = 1;
	_viewportState.pViewports = &_viewPort;

	return *this;
}

VGM::PipelineConfigurator& VGM::PipelineConfigurator::setScissorState(uint32_t width, uint32_t height, int32_t xOffset, int32_t yOffset)
{
	_scissor.extent = { width, height };
	_scissor.offset = { xOffset, yOffset };

	_viewportState.scissorCount = 1;
	_viewportState.pScissors = &_scissor;

	return *this;
}

VGM::PipelineConfigurator& VGM::PipelineConfigurator::setInputAssemblyState(VkPrimitiveTopology primitiv)
{
	_inputAssembly.topology = primitiv;
	
	return *this;
}

VGM::PipelineConfigurator& VGM::PipelineConfigurator::setRasterizerState(VkPolygonMode mode, VkFrontFace frontFace)
{
	_rasterizationState.polygonMode = mode;
	_rasterizationState.frontFace = frontFace;

	return *this;
}

VGM::PipelineConfigurator& VGM::PipelineConfigurator::setBackfaceCulling(VkCullModeFlags cullMode)
{
	_rasterizationState.cullMode = cullMode;

	return *this;
}

VGM::PipelineConfigurator& VGM::PipelineConfigurator::setDepthBias(VkBool32 depthBiasEnable, float depthBiasConstantFactor, float depthBiasSlopeFactor, float depthBiasClamp)
{
	_rasterizationState.depthBiasEnable = depthBiasEnable;
	_rasterizationState.depthBiasConstantFactor = depthBiasConstantFactor;
	_rasterizationState.depthBiasSlopeFactor = depthBiasSlopeFactor;
	_rasterizationState.depthBiasClamp = depthBiasClamp;

	return *this;
}

VGM::PipelineConfigurator& VGM::PipelineConfigurator::setMultisamplingState()
{
	return *this;
}

VGM::PipelineConfigurator& VGM::PipelineConfigurator::setDepthState(VkBool32 depthTestEnable, VkBool32 depthWriteEnable, VkCompareOp depthCompareOp)
{
	_depthStencilState.depthTestEnable = depthTestEnable;
	_depthStencilState.depthWriteEnable = depthWriteEnable;
	_depthStencilState.depthCompareOp = depthCompareOp;

	return *this;
}

VGM::PipelineConfigurator& VGM::PipelineConfigurator::setStencilState(VkBool32 stencilTestEnable, VkStencilOpState frontOp, VkStencilOpState backOp)
{
	_depthStencilState.stencilTestEnable = stencilTestEnable;
	_depthStencilState.front = frontOp;
	_depthStencilState.back = backOp;

	return *this;
}

VGM::PipelineConfigurator& VGM::PipelineConfigurator::setColorBlendingState(std::vector<VkPipelineColorBlendAttachmentState>& colorBlendAttachments)
{
	_colorBlendAttachments = colorBlendAttachments;
	_colorBlendingState.pAttachments = _colorBlendAttachments.data();
	_colorBlendingState.attachmentCount = _colorBlendAttachments.size();
	return *this;
}

VGM::PipelineConfigurator& VGM::PipelineConfigurator::addDynamicState(VkDynamicState state)
{
	_dynamicStates.push_back(state);
	_dynamicState.dynamicStateCount = _dynamicStates.size();
	_dynamicState.pDynamicStates = _dynamicStates.data();

	return *this;
}

VGM::PipelineConfigurator& VGM::PipelineConfigurator::setRenderingFormats(std::vector<VkFormat>& colorFormats, VkFormat depthFormat, VkFormat stencilFormat)
{
	_colorFormats = colorFormats;

	_renderingState.pColorAttachmentFormats = _colorFormats.data();
	_renderingState.colorAttachmentCount = _colorFormats.size();
	_renderingState.depthAttachmentFormat = depthFormat;
	_renderingState.stencilAttachmentFormat = stencilFormat;
	_renderingState.viewMask = 0;

	return *this;
}

VkGraphicsPipelineCreateInfo VGM::PipelineConfigurator::getPipelineConfiguration()
{
	_vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
	_vertexInputInfo.pNext = nullptr;
	_vertexInputInfo.pVertexBindingDescriptions = _vertexBindingDescriptions.data();
	_vertexInputInfo.vertexBindingDescriptionCount = _vertexBindingDescriptions.size();
	_vertexInputInfo.pVertexAttributeDescriptions = _vertexAttributeDescriptons.data();
	_vertexInputInfo.vertexAttributeDescriptionCount = _vertexAttributeDescriptons.size();

	VkGraphicsPipelineCreateInfo info;
	info.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
	info.pColorBlendState = &_colorBlendingState;
	info.pDepthStencilState = &_depthStencilState;
	info.pDynamicState = &_dynamicState;
	info.pInputAssemblyState = &_inputAssembly;
	info.pMultisampleState = &_multisampleState;
	info.pRasterizationState = &_rasterizationState;
	//info.pTessellationState = &_tesselationState;
	info.pVertexInputState = &_vertexInputInfo;
	info.pViewportState = &_viewportState;
	info.pNext = &_renderingState;
	info.basePipelineHandle = VK_NULL_HANDLE;
	info.renderPass = nullptr;
	info.flags = 0;

	return info;
}
