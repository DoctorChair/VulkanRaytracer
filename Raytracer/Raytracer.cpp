#include "Raytracer.h"

void Raytracer::init(uint32_t windowWidth, uint32_t windowHeight)
{
	SDL_Init(SDL_INIT_VIDEO);
	SDL_Window* window = SDL_CreateWindow("Vulkan", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
		windowWidth, windowHeight,
		SDL_WINDOW_VULKAN | SDL_WINDOW_RESIZABLE);

	VkPhysicalDeviceVulkan13Features features13 = {};
	features13.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES;
	features13.pNext = nullptr;
	features13.dynamicRendering = true;

	this->windowWidth = windowWidth;
	this->windowHeight = windowHeight;

	renderContext = VGM::VulkanContext(window, 1, 3, 0,
		{},
		{ "VK_LAYER_KHRONOS_validation" },
		{ VK_KHR_SWAPCHAIN_EXTENSION_NAME }, &features13);

	initVertexBuffer();
	initTextureArrays();
	initDescriptorSetAllocator();
	initgBufferDescriptorSets();
	initgBuffers();
	initGBufferShader();
	initSyncStructures();
	initCommandBuffers();
	initDataBuffers();
}

void Raytracer::loadeMesh()
{

}

void Raytracer::execute()
{
	VkFence renderFence = _frameSynchroStructs[_currentFrameIndex]._renderFence;
	VkSemaphore renderSemaphore = _frameSynchroStructs[_currentFrameIndex]._renderSemaphore;
	VkSemaphore availabilitySemaphore = _frameSynchroStructs[_currentFrameIndex]._availabilitySemaphore;
	
	VGM::CommandBuffer* cmd = &_renderCommandBuffers[_currentFrameIndex];
	VGM::Buffer* currentGlobalRenderDataBuffer = &_globalRenderDataBuffers[_currentFrameIndex];
	VGM::Buffer* currentCameraBuffer = &_cameraBuffers[_currentFrameIndex];
	VGM::Buffer* currentDrawDataInstanceBuffer = &_drawDataIsntanceBuffers[_currentFrameIndex];

	VkDescriptorSet level0DescriptorSet;
	VkDescriptorSet level1DescriptorSet;
	VkDescriptorSet level2DescriptorSet;

	VkDescriptorBufferInfo globalbufferInfo;
	globalbufferInfo.buffer = currentGlobalRenderDataBuffer->get();
	globalbufferInfo.offset = 0;
	globalbufferInfo.range = currentGlobalRenderDataBuffer->size();

	VkDescriptorBufferInfo camerabufferInfo;
	camerabufferInfo.buffer = currentCameraBuffer->get();
	camerabufferInfo.offset = 0;
	camerabufferInfo.range = currentCameraBuffer->size();

	VGM::DescriptorSetBuilder::begin()
		.bindBuffer(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, globalbufferInfo)
		.bindBuffer(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, camerabufferInfo)
		.createDescriptorSet(renderContext._device, &level0DescriptorSet, &level0Layout, _descriptorSetAllocator);

	VkDescriptorImageInfo albedoInfo;
	albedoInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	albedoInfo.imageView = _albedoTextureView;
	albedoInfo.sampler = _albedoSampler;

	VkDescriptorImageInfo metallicInfo;
	metallicInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	metallicInfo.imageView = _metallicTextureView;
	metallicInfo.sampler = _metallicSampler;

	VkDescriptorImageInfo normalInfo;
	normalInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	normalInfo.imageView = _normalTextureView;
	normalInfo.sampler = _normalSampler;

	VkDescriptorImageInfo roughnessInfo;
	roughnessInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	roughnessInfo.imageView = _roughnessTextureView;
	roughnessInfo.sampler = _roughnessSampler;

	VGM::DescriptorSetBuilder::begin()
		.bindImage(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, albedoInfo, nullptr)
		.bindImage(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, metallicInfo, nullptr)
		.bindImage(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, normalInfo, nullptr)
		.bindImage(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, roughnessInfo, nullptr)
		.createDescriptorSet(renderContext._device, &level1DescriptorSet, &level1Layout, _descriptorSetAllocator);
	
	VkDescriptorBufferInfo drawDataInstanceInfo;
	drawDataInstanceInfo.buffer = currentDrawDataInstanceBuffer->get();
	drawDataInstanceInfo.offset = 0;
	drawDataInstanceInfo.range = currentDrawDataInstanceBuffer->size();

	VGM::DescriptorSetBuilder::begin()
		.bindBuffer(VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, drawDataInstanceInfo)
		.createDescriptorSet(renderContext._device, &level2DescriptorSet, &level2Layout, _descriptorSetAllocator);

	VkDescriptorSet descriptorSets[] = { level0DescriptorSet, level1DescriptorSet, level2DescriptorSet };

	renderContext.aquireNextImageIndex(nullptr);

	cmd->begin(&renderFence, 1, renderContext._device);

	_gBufferShader.cmdBind(cmd->get());
	vkCmdBindDescriptorSets(cmd->get(), VK_PIPELINE_BIND_POINT_GRAPHICS, _gBufferPipelineLayout, 0, 3, descriptorSets, 0, nullptr);
	//drawing here


	cmd->end();
	
	VkPipelineStageFlags waitFlag = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
	cmd->submit(&renderSemaphore, 1, &availabilitySemaphore, 1, &waitFlag, renderFence, renderContext._graphicsQeueu);
	
	_currentFrameIndex = (_currentFrameIndex + 1) % _maxBuffers;
}

void Raytracer::destroy()
{
	renderContext.destroy();
}

void Raytracer::initVertexBuffer()
{
	_vertexBuffer.indices = VGM::Buffer(VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, 100, renderContext._gpuAllocator);
}

void Raytracer::initTextureArrays()
{
	VkExtent3D textureDimensions = { 1024, 1024, 1 };

	_albedoTextureArray = VGM::Texture(VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_TYPE_2D,
		VK_IMAGE_USAGE_SAMPLED_BIT, textureDimensions, 100, 3, renderContext._gpuAllocator);
	_albedoTextureArray.createImageView(0, 3, 0, 100, renderContext._device, &_albedoTextureView);

	_metallicTextureArray = VGM::Texture(VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_TYPE_2D,
		VK_IMAGE_USAGE_SAMPLED_BIT, textureDimensions, 100, 3, renderContext._gpuAllocator);
	_metallicTextureArray.createImageView(0, 3, 0, 100, renderContext._device, &_metallicTextureView);

	_normalTextureArray = VGM::Texture(VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_TYPE_2D,
		VK_IMAGE_USAGE_SAMPLED_BIT, textureDimensions, 100, 3, renderContext._gpuAllocator);
	_normalTextureArray.createImageView(0, 3, 0, 100, renderContext._device, &_normalTextureView);

	_roughnessTextureArray = VGM::Texture(VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_TYPE_2D,
		VK_IMAGE_USAGE_SAMPLED_BIT, textureDimensions, 100, 3, renderContext._gpuAllocator);
	_roughnessTextureArray.createImageView(0, 3, 0, 100, renderContext._device, &_roughnessTextureView);

	VkSamplerCreateInfo createInfo = {};
	createInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
	createInfo.pNext = nullptr;
	createInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
	createInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
	createInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
	createInfo.anisotropyEnable = VK_FALSE;
	createInfo.borderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_BLACK;
	createInfo.compareEnable = VK_FALSE;
	createInfo.flags = 0;
	createInfo.minFilter = VK_FILTER_LINEAR;
	createInfo.magFilter = VK_FILTER_LINEAR;
	createInfo.maxAnisotropy = 8.0f;
	createInfo.maxLod = 3;
	createInfo.minLod = 0;
	createInfo.mipLodBias = 0;
	createInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
	createInfo.unnormalizedCoordinates = VK_FALSE;

	vkCreateSampler(renderContext._device, &createInfo, nullptr, &_albedoSampler);
	vkCreateSampler(renderContext._device, &createInfo, nullptr, &_metallicSampler);
	vkCreateSampler(renderContext._device, &createInfo, nullptr, &_normalSampler);
	vkCreateSampler(renderContext._device, &createInfo, nullptr, &_roughnessSampler);
}

void Raytracer::initgBuffers()
{
	_gBufferChain.resize(_maxBuffers);
	for (auto& g : _gBufferChain)
	{
		g.positionBuffer = VGM::Texture(VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_TYPE_2D, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
			{ windowWidth, windowHeight, 1 }, 1, 1, renderContext._gpuAllocator);
		g.normalBuffer = VGM::Texture(VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_TYPE_2D, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
			{ windowWidth, windowHeight, 1 }, 1, 1, renderContext._gpuAllocator);
		g.idBuffer =  VGM::Texture(VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_TYPE_2D, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
			{ windowWidth, windowHeight, 1 }, 1, 1, renderContext._gpuAllocator);
		g.albedoBuffer = VGM::Texture(VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_TYPE_2D, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
			{ windowWidth, windowHeight, 1 }, 1, 1, renderContext._gpuAllocator);

		VkClearColorValue clear = { 0, 0, 0 };

		g.positionBuffer.createImageView(0, 1, 0, 1, renderContext._device, &g.positionView);
		g.framebuffer.bindColorTextureTarget(g.positionView, VK_ATTACHMENT_LOAD_OP_CLEAR, VK_ATTACHMENT_STORE_OP_STORE, clear);

		g.normalBuffer.createImageView(0, 1, 0, 1, renderContext._device, &g.normalView);
		g.framebuffer.bindColorTextureTarget(g.normalView, VK_ATTACHMENT_LOAD_OP_CLEAR, VK_ATTACHMENT_STORE_OP_STORE, clear);

		g.idBuffer.createImageView(0, 1, 0, 1, renderContext._device, &g.idView);
		g.framebuffer.bindColorTextureTarget(g.idView, VK_ATTACHMENT_LOAD_OP_CLEAR, VK_ATTACHMENT_STORE_OP_STORE, clear);

		g.albedoBuffer.createImageView(0, 1, 0, 1, renderContext._device, &g.albedoView);
		g.framebuffer.bindColorTextureTarget(g.albedoView, VK_ATTACHMENT_LOAD_OP_CLEAR, VK_ATTACHMENT_STORE_OP_STORE, clear);
	}
}

void Raytracer::initDescriptorSetAllocator()
{
	std::vector<VkDescriptorPoolSize> poolSizes = {
	{VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 10},
	{VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 10},
	{VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 10},
	};
	_descriptorSetAllocator = VGM::DescriptorSetAllocator(poolSizes, 100, 0, 10, renderContext._device);
}

void Raytracer::initgBufferDescriptorSets()
{
	VGM::DescriptorSetLayoutBuilder::begin()
		.addBinding(0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, nullptr, VK_SHADER_STAGE_ALL_GRAPHICS)
		.addBinding(1, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, nullptr, VK_SHADER_STAGE_ALL_GRAPHICS)
		.createDescriptorSetLayout(renderContext._device, &level0Layout);

	VGM::DescriptorSetLayoutBuilder::begin()
		.addBinding(0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, nullptr, VK_SHADER_STAGE_ALL_GRAPHICS)
		.addBinding(1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, nullptr, VK_SHADER_STAGE_ALL_GRAPHICS)
		.addBinding(2, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, nullptr, VK_SHADER_STAGE_ALL_GRAPHICS)
		.addBinding(3, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, nullptr, VK_SHADER_STAGE_ALL_GRAPHICS)
		.createDescriptorSetLayout(renderContext._device, &level1Layout);

	VGM::DescriptorSetLayoutBuilder::begin()
		.addBinding(0, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, nullptr, VK_SHADER_STAGE_ALL_GRAPHICS)
		.createDescriptorSetLayout(renderContext._device, &level2Layout);
}

void Raytracer::initGBufferShader()
{
	VkDescriptorSetLayout setLayouts[] = { level0Layout, level1Layout, level2Layout };

	VkPipelineLayoutCreateInfo createInfo = {};
	createInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	createInfo.pNext = nullptr;
	createInfo.flags = 0;
	createInfo.pushConstantRangeCount = 0;
	createInfo.setLayoutCount = 3;
	createInfo.pSetLayouts = setLayouts;

	vkCreatePipelineLayout(renderContext._device, &createInfo, nullptr, &_gBufferPipelineLayout);

	std::vector<std::pair<std::string, VkShaderStageFlagBits>> sources =
	{
		{"C:\\Users\\Eric\\projects\\VulkanRaytracing\\shaders\\SPIRV\\gBufferVERT.spv", VK_SHADER_STAGE_VERTEX_BIT},
		{"C:\\Users\\Eric\\projects\\VulkanRaytracing\\shaders\\SPIRV\\gBufferFRAG.spv", VK_SHADER_STAGE_FRAGMENT_BIT}
	};

	VGM::PipelineConfigurator configurator;
	std::vector<VkFormat> renderingColorFormats = { VK_FORMAT_R8G8B8A8_SRGB, VK_FORMAT_R8G8B8A8_SRGB, VK_FORMAT_R8G8B8A8_SRGB, VK_FORMAT_R8G8B8A8_SRGB };

	VkPipelineColorBlendAttachmentState colorBlendAttachment = {};
	colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
	colorBlendAttachment.blendEnable = VK_FALSE;
	std::vector<VkPipelineColorBlendAttachmentState> colorBlendAttachments = { colorBlendAttachment, colorBlendAttachment, colorBlendAttachment, colorBlendAttachment };

	configurator.setRenderingFormats(renderingColorFormats, VK_FORMAT_D32_SFLOAT, VK_FORMAT_UNDEFINED);
	configurator.setViewportState(windowWidth, windowHeight, 0.0f, 1.0f, 0.0f, 0.0f);
	configurator.setScissorState(windowWidth, windowHeight, 0.0f, 0.0f);
	configurator.setInputAssemblyState(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST);
	configurator.setColorBlendingState(colorBlendAttachments);
	//configurator.setDepthState(VK_TRUE, VK_TRUE, VK_COMPARE_OP_LESS_OR_EQUAL);

	configurator.addVertexAttribInputDescription(0, 0, VK_FORMAT_R32G32B32_SFLOAT, 0); //Position
	configurator.addVertexAttribInputDescription(1, 0, VK_FORMAT_R32G32_SFLOAT, 3 * sizeof(float)); //texCoord
	configurator.addVertexAttribInputDescription(2, 0, VK_FORMAT_R32G32B32_SFLOAT, 5 * sizeof(float)); //normal
	configurator.addVertexAttribInputDescription(3, 0, VK_FORMAT_R32G32B32_SFLOAT, 8 * sizeof(float)); //tangent
	configurator.addVertexAttribInputDescription(4, 0, VK_FORMAT_R32G32B32_SFLOAT, 11 * sizeof(float)); //bitangant
	configurator.addVertexInputInputBindingDescription(0, 14 * sizeof(float), VK_VERTEX_INPUT_RATE_VERTEX);

	_gBufferShader = VGM::ShaderProgram(sources, _gBufferPipelineLayout, configurator, renderContext._device);
}

void Raytracer::initCommandBuffers()
{
	_renderCommandBufferAllocator = VGM::CommandBufferAllocator(renderContext._graphicsQueueFamilyIndex, renderContext._device);
	
	_renderCommandBuffers.resize(_maxBuffers);
	for(auto& c : _renderCommandBuffers)
	{
		c = VGM::CommandBuffer(_renderCommandBufferAllocator, renderContext._device);
	}
}

void Raytracer::initSyncStructures()
{
	VkFenceCreateInfo fenceCreateInfo = {};
	fenceCreateInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
	fenceCreateInfo.pNext = nullptr;
	fenceCreateInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

	VkSemaphoreCreateInfo semaphoreCreateInfo = {};
	semaphoreCreateInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
	semaphoreCreateInfo.pNext = nullptr;
	semaphoreCreateInfo.flags = 0;

	_frameSynchroStructs.resize(_maxBuffers);
	for (unsigned int i = 0; i < _maxBuffers; i++)
	{
		vkCreateFence(renderContext._device, &fenceCreateInfo, nullptr, &_frameSynchroStructs[i]._renderFence);
		vkCreateSemaphore(renderContext._device, &semaphoreCreateInfo, nullptr, &_frameSynchroStructs[i]._renderSemaphore);
		vkCreateSemaphore(renderContext._device, &semaphoreCreateInfo, nullptr, &_frameSynchroStructs[i]._availabilitySemaphore);
	}
}

void Raytracer::initDataBuffers()
{
	_drawDataIsntanceBuffers.resize(_maxBuffers);
	_cameraBuffers.resize(_maxBuffers);
	_globalRenderDataBuffers.resize(_maxBuffers);

	for(unsigned int i = 0; i<_maxBuffers; i++)
	{
		_drawDataIsntanceBuffers[i] = VGM::Buffer(VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, _maxDrawCount * sizeof(DrawData), renderContext._gpuAllocator);
		_cameraBuffers[i] = VGM::Buffer(VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, sizeof(CameraData), renderContext._gpuAllocator);
		_globalRenderDataBuffers[i] = VGM::Buffer(VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, sizeof(globalRenderData), renderContext._gpuAllocator);
	}
}
