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

	initDescripotrSetAllocator();

	initgBufferDescriptorSets();

	initgBuffers();

	initGBufferShader();

	initSyncStructures();

	initCommandBuffers();
}

void Raytracer::loadeMesh()
{
}

void Raytracer::execute()
{

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

	_metallicTextureArray = VGM::Texture(VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_TYPE_2D,
		VK_IMAGE_USAGE_SAMPLED_BIT, textureDimensions, 100, 3, renderContext._gpuAllocator);

	_normalTextureArray = VGM::Texture(VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_TYPE_2D,
		VK_IMAGE_USAGE_SAMPLED_BIT, textureDimensions, 100, 3, renderContext._gpuAllocator);

	_roughnessTextureArray = VGM::Texture(VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_TYPE_2D,
		VK_IMAGE_USAGE_SAMPLED_BIT, textureDimensions, 100, 3, renderContext._gpuAllocator);
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

void Raytracer::initDescripotrSetAllocator()
{
	std::vector<VkDescriptorPoolSize> poolSizes = {
	{VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 10},
	{VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 10},
	};
	_descriptorSetAllocator = VGM::DescriptorSetAllocator(poolSizes, 100, 0, 10, renderContext._device);
}

void Raytracer::initgBufferDescriptorSets()
{
}

void Raytracer::initGBufferShader()
{
	VkPipelineLayoutCreateInfo createInfo = {};
	createInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	createInfo.pNext = nullptr;
	createInfo.flags = 0;
	createInfo.pushConstantRangeCount = 0;
	createInfo.setLayoutCount = 0;

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

	_renderFences.resize(_maxBuffers);
	_renderSeamphores.resize(_maxBuffers);
	_avaialableSemaphore.resize(_maxBuffers);
	for (unsigned int i = 0; i < _maxBuffers; i++)
	{
		vkCreateFence(renderContext._device, &fenceCreateInfo, nullptr, &_renderFences[i]);
		vkCreateSemaphore(renderContext._device, &semaphoreCreateInfo, nullptr, &_renderSeamphores[i]);
		vkCreateSemaphore(renderContext._device, &semaphoreCreateInfo, nullptr, &_avaialableSemaphore[i]);
	}
}
