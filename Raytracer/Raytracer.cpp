#include "Raytracer.h"

void Raytracer::init(SDL_Window* window)
{
	int w = 0;
	int h = 0;
	SDL_GetWindowSize(window, &w, &h);

	windowWidth = w;
	windowHeight = h;

	_concurrencyCount = 3;

	VkPhysicalDeviceVulkan13Features features13 = {};
	features13.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES;
	features13.pNext = nullptr;
	features13.dynamicRendering = true;

	renderContext = VGM::VulkanContext(window, 1, 3, 0,
		{},
		{ "VK_LAYER_KHRONOS_validation" },
		{ VK_KHR_SWAPCHAIN_EXTENSION_NAME }, &features13, _concurrencyCount);

	initPresentFramebuffers();
	initVertexBuffer();
	initTextureArrays();
	initDescriptorSetAllocator();
	initgBufferDescriptorSets();
	initgBuffers();
	initGBufferShader();
	initDefferedShader();
	initSyncStructures();
	initCommandBuffers();
	initDataBuffers();
}

Mesh Raytracer::loadMesh()
{
	Mesh mesh;

	VkFence transferFence;
	VkFenceCreateInfo createInfo = {};
	createInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
	createInfo.pNext = nullptr;
	createInfo.flags = 0;

	vkCreateFence(renderContext._device, &createInfo, nullptr, &transferFence);

	VGM::Buffer vertexTransferBuffer(VK_BUFFER_USAGE_TRANSFER_SRC_BIT, 1, renderContext._gpuAllocator);
	VGM::Buffer indexTransferBuffer(VK_BUFFER_USAGE_TRANSFER_SRC_BIT, 1 * sizeof(uint32_t), renderContext._gpuAllocator);

	_transferCommandBuffer.begin(nullptr, 0, renderContext._device);

	VkBufferCopy vertexRegion;
	VkBufferCopy indexRegion;

	_vertexBuffer.vertices.cmdCopyBuffer(_transferCommandBuffer.get(), vertexTransferBuffer.get(), 1, &vertexRegion);
	_vertexBuffer.indices.cmdCopyBuffer(_transferCommandBuffer.get(), indexTransferBuffer.get(), 1, &indexRegion);

	_transferCommandBuffer.end();
	VkPipelineStageFlags stageFlags = { VK_PIPELINE_STAGE_VERTEX_INPUT_BIT };
	_transferCommandBuffer.submit(nullptr, 0, nullptr, 0, &stageFlags, transferFence, renderContext._transferQeueu);
	vkWaitForFences(renderContext._device, 1, &transferFence, VK_TRUE, 99999999);
	_transferCommandBufferAllocator.reset(renderContext._device);
	vkDestroyFence(renderContext._device, transferFence, nullptr);

	return mesh;
}

void Raytracer::drawMesh(Mesh mesh, glm::mat4 transform)
{
	_drawDataTransferCache.push_back({ transform, mesh.material });
	VkDrawIndexedIndirectCommand command;
	command.vertexOffset = mesh.vertexOffset;
	command.instanceCount = 1;
	command.indexCount = mesh.indicesCount;
	command.firstIndex = mesh.indexOffset;
	command.firstInstance = 0;
	_drawCommandTransferCache.push_back(command);
}

void Raytracer::update()
{
	VkFence offscreenRenderFence = _frameSynchroStructs[_currentFrameIndex]._offsrceenRenderFence;
	VkFence defferendRenderFence = _frameSynchroStructs[_currentFrameIndex]._defferedRenderFence;
	VkSemaphore offsceenRenderSemaphore = _frameSynchroStructs[_currentFrameIndex]._offsceenRenderSemaphore;
	VkSemaphore defferedRenderSemaphore = _frameSynchroStructs[_currentFrameIndex]._defferedRenderSemaphore;
	VkSemaphore availabilitySemaphore = _frameSynchroStructs[_currentFrameIndex]._availabilitySemaphore;
	VkSemaphore presentSemaphore = _frameSynchroStructs[_currentFrameIndex]._presentSemaphore;

	VGM::CommandBuffer* offscreenCmd = &_offsceenRenderCommandBuffers[_currentFrameIndex];
	VGM::CommandBuffer* defferedCmd = &_defferedRenderCommandBuffers[_currentFrameIndex];

	VGM::Buffer* currentGlobalRenderDataBuffer = &_globalRenderDataBuffers[_currentFrameIndex];
	VGM::Buffer* currentCameraBuffer = &_cameraBuffers[_currentFrameIndex];
	VGM::Buffer* currentDrawDataInstanceBuffer = &_drawDataIsntanceBuffers[_currentFrameIndex];

	VGM::Buffer* currentDrawIndirectBuffer = &_drawIndirectCommandBuffer[_currentFrameIndex];

	GBuffer* currentGBuffer = &_gBufferChain[_currentFrameIndex];
	
	VGM::DescriptorSetAllocator* currentOffsceenDescriptorSetAllocator = &_offsecreenDescriptorSetAllocators[_currentFrameIndex];
	VGM::DescriptorSetAllocator* currentDefferedDescriptorSetAllocator = &_defferedDescriptorSetAllocators[_currentFrameIndex];

	VkDescriptorSet level0DescriptorSet;
	VkDescriptorSet level1DescriptorSet;
	VkDescriptorSet level2DescriptorSet;

	VkRect2D renderArea;
	renderArea.extent = { windowWidth, windowHeight };
	renderArea.offset = { 0, 0 };

	//upload transfer data here
	uint32_t swapchainIndex = renderContext.aquireNextImageIndex(nullptr, presentSemaphore);

	//OffscreenRenderPass
	VGM::Framebuffer* currentPresentFramebuffer = &_presentFramebuffers[swapchainIndex];

	offscreenCmd->begin(&offscreenRenderFence, 1, renderContext._device);

	_drawIndirectCommandBuffer[_currentFrameIndex].uploadData(_drawCommandTransferCache.data(),
		_drawCommandTransferCache.size() * sizeof(VkDrawIndexedIndirectCommand),
		0, renderContext._gpuAllocator);
	_drawDataIsntanceBuffers[_currentFrameIndex].uploadData(_drawDataTransferCache.data(),
		_drawDataTransferCache.size() * sizeof(DrawData),
		0, renderContext._gpuAllocator);

	_drawCommandTransferCache.clear();
	_drawDataTransferCache.clear();

	currentOffsceenDescriptorSetAllocator->resetPools(renderContext._device);

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
		.createDescriptorSet(renderContext._device, &level0DescriptorSet, &level0Layout, *currentOffsceenDescriptorSetAllocator);

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
		.createDescriptorSet(renderContext._device, &level1DescriptorSet, &level1Layout, *currentOffsceenDescriptorSetAllocator);

	VkDescriptorBufferInfo drawDataInstanceInfo;
	drawDataInstanceInfo.buffer = currentDrawDataInstanceBuffer->get();
	drawDataInstanceInfo.offset = 0;
	drawDataInstanceInfo.range = currentDrawDataInstanceBuffer->size();

	VGM::DescriptorSetBuilder::begin()
		.bindBuffer(VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, drawDataInstanceInfo)
		.createDescriptorSet(renderContext._device, &level2DescriptorSet, &level2Layout, *currentOffsceenDescriptorSetAllocator);

	VkDescriptorSet descriptorSets[] = { level0DescriptorSet, level1DescriptorSet, level2DescriptorSet };

	currentGBuffer->albedoBuffer.cmdPrepareTextureForFragmentShaderWrite(offscreenCmd->get());
	currentGBuffer->idBuffer.cmdPrepareTextureForFragmentShaderWrite(offscreenCmd->get());
	currentGBuffer->normalBuffer.cmdPrepareTextureForFragmentShaderWrite(offscreenCmd->get());
	currentGBuffer->positionBuffer.cmdPrepareTextureForFragmentShaderWrite(offscreenCmd->get());
	currentGBuffer->depthBuffer.cmdPrepareTextureForFragmentShaderWrite(offscreenCmd->get());
	currentGBuffer->framebuffer.cmdBeginRendering(offscreenCmd->get(), renderArea);
	
	//drawCommandHere
	_gBufferShader.cmdBind(offscreenCmd->get());
	vkCmdBindDescriptorSets(offscreenCmd->get(), VK_PIPELINE_BIND_POINT_GRAPHICS, _gBufferPipelineLayout, 0, 3, descriptorSets, 0, nullptr);
	vkCmdDraw(offscreenCmd->get(), 3, 1, 0, 0);

	//vkCmdDrawIndexedIndirect(offscreenCmd->get(), currentDrawIndirectBuffer->get(), 0, 1, sizeof(VkDrawIndexedIndirectCommand));

	currentGBuffer->framebuffer.cmdEndRendering(offscreenCmd->get());
	currentGBuffer->albedoBuffer.cmdPrepareTextureForFragmentShaderRead(offscreenCmd->get());
	currentGBuffer->idBuffer.cmdPrepareTextureForFragmentShaderRead(offscreenCmd->get());
	currentGBuffer->normalBuffer.cmdPrepareTextureForFragmentShaderRead(offscreenCmd->get());
	currentGBuffer->positionBuffer.cmdPrepareTextureForFragmentShaderRead(offscreenCmd->get());
	currentGBuffer->depthBuffer.cmdPrepareTextureForFragmentShaderRead(offscreenCmd->get());

	offscreenCmd->end();
	VkPipelineStageFlags waitFlag = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
	offscreenCmd->submit(&offsceenRenderSemaphore, 1, &presentSemaphore, 1, &waitFlag, offscreenRenderFence, renderContext._graphicsQeueu);
	


	//DefferedRenderPass
	defferedCmd = &_defferedRenderCommandBuffers[_currentFrameIndex];
	defferedCmd->begin(&defferendRenderFence, 1, renderContext._device);
	currentDefferedDescriptorSetAllocator->resetPools(renderContext._device);

	VkDescriptorSet defferedDescriptorSet;
	VkDescriptorImageInfo positionInfo;
	positionInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	positionInfo.imageView = currentGBuffer->positionView;
	positionInfo.sampler = _gBufferPositionSampler;
	VGM::DescriptorSetBuilder::begin()
		.bindImage(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, positionInfo, nullptr)
		.bindImage(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, positionInfo, nullptr)
		.bindImage(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, positionInfo, nullptr)
		.bindImage(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, positionInfo, nullptr)
		.createDescriptorSet(renderContext._device, &defferedDescriptorSet, &_defferedLayout, *currentDefferedDescriptorSetAllocator);

	renderContext.cmdPrepareSwapchainImageForRendering(defferedCmd->get());
	currentPresentFramebuffer->cmdBeginRendering(defferedCmd->get(), renderArea);
	
	//defferdRenderingHere
	_defferedShader.cmdBind(defferedCmd->get());
	vkCmdBindDescriptorSets(defferedCmd->get(), VK_PIPELINE_BIND_POINT_GRAPHICS, _defferedPipelineLayout, 0, 1, &defferedDescriptorSet, 0, nullptr);
	vkCmdDraw(defferedCmd->get(), 3, 1, 0, 0);

	currentPresentFramebuffer->cmdEndRendering(defferedCmd->get());
	renderContext.cmdPrepareSwaphainImageForPresent(defferedCmd->get());
	defferedCmd->end();
	defferedCmd->submit(&defferedRenderSemaphore, 1, &offsceenRenderSemaphore, 1, &waitFlag, defferendRenderFence, renderContext._graphicsQeueu);

	renderContext.presentImage(&defferedRenderSemaphore, 1);

	_currentFrameIndex = (_currentFrameIndex + 1) % _concurrencyCount;
}

void Raytracer::destroy()
{
	renderContext.destroy();
}

void Raytracer::initPresentFramebuffers()
{
	_presentFramebuffers.resize(_concurrencyCount);
	VkClearColorValue clearColor = { 1.0f, 0.0f, 0.0f, 0.0f };
	for(unsigned int i = 0; i<_concurrencyCount; i++)
	{
		_presentFramebuffers[i].bindColorTextureTarget(renderContext._swapchainImageViews[i], VK_ATTACHMENT_LOAD_OP_CLEAR, VK_ATTACHMENT_STORE_OP_STORE, clearColor);
	}
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

	createInfo.maxLod = 0;
	vkCreateSampler(renderContext._device, &createInfo, nullptr, &_gBufferPositionSampler);
}

void Raytracer::initgBuffers()
{
	_gBufferChain.resize(_concurrencyCount);
	for (auto& g : _gBufferChain)
	{
		g.positionBuffer = VGM::Texture(VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_TYPE_2D, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
			{ windowWidth, windowHeight, 1 }, 1, 1, renderContext._gpuAllocator);
		g.normalBuffer = VGM::Texture(VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_TYPE_2D, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
			{ windowWidth, windowHeight, 1 }, 1, 1, renderContext._gpuAllocator);
		g.idBuffer =  VGM::Texture(VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_TYPE_2D, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
			{ windowWidth, windowHeight, 1 }, 1, 1, renderContext._gpuAllocator);
		g.albedoBuffer = VGM::Texture(VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_TYPE_2D, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
			{ windowWidth, windowHeight, 1 }, 1, 1, renderContext._gpuAllocator);
		g.depthBuffer = VGM::Texture(VK_FORMAT_D32_SFLOAT, VK_IMAGE_TYPE_2D, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
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
	
		g.depthBuffer.createImageView(0, 1, 0, 1, renderContext._device, &g.depthView);
		g.framebuffer.bindDepthTextureTarget(g.depthView, VK_ATTACHMENT_LOAD_OP_CLEAR, VK_ATTACHMENT_STORE_OP_STORE, 0.0f);
	}
}

void Raytracer::initDescriptorSetAllocator()
{
	std::vector<VkDescriptorPoolSize> poolSizes = {
	{VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 10},
	{VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 10},
	{VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 10},
	};
	
	_offsecreenDescriptorSetAllocators.resize(_concurrencyCount);
	_defferedDescriptorSetAllocators.resize(_concurrencyCount);
	for(auto& d : _offsecreenDescriptorSetAllocators)
	{
		d = VGM::DescriptorSetAllocator(poolSizes, 100, 0, 10, renderContext._device);
	}
	for (auto& d : _defferedDescriptorSetAllocators)
	{
		d = VGM::DescriptorSetAllocator(poolSizes, 100, 0, 10, renderContext._device);
	}
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

	VGM::DescriptorSetLayoutBuilder::begin()
		.addBinding(0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, nullptr, VK_SHADER_STAGE_ALL_GRAPHICS)
		.addBinding(1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, nullptr, VK_SHADER_STAGE_ALL_GRAPHICS)
		.addBinding(2, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, nullptr, VK_SHADER_STAGE_ALL_GRAPHICS)
		.addBinding(3, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, nullptr, VK_SHADER_STAGE_ALL_GRAPHICS)
		.createDescriptorSetLayout(renderContext._device, &_defferedLayout);
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
	configurator.setDepthState(VK_TRUE, VK_TRUE, VK_COMPARE_OP_LESS_OR_EQUAL);

	//configurator.addVertexAttribInputDescription(0, 0, VK_FORMAT_R32G32B32_SFLOAT, 0); //Position
	//configurator.addVertexAttribInputDescription(1, 0, VK_FORMAT_R32G32_SFLOAT, 3 * sizeof(float)); //texCoord
	//configurator.addVertexAttribInputDescription(2, 0, VK_FORMAT_R32G32B32_SFLOAT, 5 * sizeof(float)); //normal
	//configurator.addVertexAttribInputDescription(3, 0, VK_FORMAT_R32G32B32_SFLOAT, 8 * sizeof(float)); //tangent
	//configurator.addVertexAttribInputDescription(4, 0, VK_FORMAT_R32G32B32_SFLOAT, 11 * sizeof(float)); //bitangant
	//configurator.addVertexInputInputBindingDescription(0, 14 * sizeof(float), VK_VERTEX_INPUT_RATE_VERTEX);

	_gBufferShader = VGM::ShaderProgram(sources, _gBufferPipelineLayout, configurator, renderContext._device);
}

void Raytracer::initDefferedShader()
{
	VkDescriptorSetLayout setLayouts[] = {_defferedLayout};

	VkPipelineLayoutCreateInfo createInfo = {};
	createInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	createInfo.pNext = nullptr;
	createInfo.flags = 0;
	createInfo.pushConstantRangeCount = 0;
	createInfo.setLayoutCount = 1;
	createInfo.pSetLayouts = &_defferedLayout;
	
	vkCreatePipelineLayout(renderContext._device, &createInfo, nullptr, &_defferedPipelineLayout);

	std::vector<std::pair<std::string, VkShaderStageFlagBits>> sources =
	{
		{"C:\\Users\\Eric\\projects\\VulkanRaytracing\\shaders\\SPIRV\\defferedVERT.spv", VK_SHADER_STAGE_VERTEX_BIT},
		{"C:\\Users\\Eric\\projects\\VulkanRaytracing\\shaders\\SPIRV\\defferedFRAG.spv", VK_SHADER_STAGE_FRAGMENT_BIT}
	};

	VGM::PipelineConfigurator configurator;
	std::vector<VkFormat> renderingColorFormats = { renderContext._swapchainFormat.format};

	VkPipelineColorBlendAttachmentState colorBlendAttachment = {};
	colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
	colorBlendAttachment.blendEnable = VK_FALSE;
	std::vector<VkPipelineColorBlendAttachmentState> colorBlendAttachments = { colorBlendAttachment};

	configurator.setRenderingFormats(renderingColorFormats, VK_FORMAT_UNDEFINED, VK_FORMAT_UNDEFINED);
	configurator.setViewportState(windowWidth, windowHeight, 0.0f, 1.0f, 0.0f, 0.0f);
	configurator.setScissorState(windowWidth, windowHeight, 0.0f, 0.0f);
	configurator.setInputAssemblyState(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST);
	configurator.setBackfaceCulling(VK_CULL_MODE_NONE);
	configurator.setColorBlendingState(colorBlendAttachments);

	_defferedShader = VGM::ShaderProgram(sources, _defferedPipelineLayout, configurator, renderContext._device);
}

void Raytracer::initCommandBuffers()
{
	_renderCommandBufferAllocator = VGM::CommandBufferAllocator(renderContext._graphicsQueueFamilyIndex, renderContext._device);
	
	_offsceenRenderCommandBuffers.resize(_concurrencyCount);
	for(auto& c : _offsceenRenderCommandBuffers)
	{
		c = VGM::CommandBuffer(_renderCommandBufferAllocator, renderContext._device);
	}

	_defferedRenderCommandBuffers.resize(_concurrencyCount);
	for (auto& c : _defferedRenderCommandBuffers)
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

	_frameSynchroStructs.resize(_concurrencyCount);
	for (unsigned int i = 0; i < _concurrencyCount; i++)
	{
		vkCreateFence(renderContext._device, &fenceCreateInfo, nullptr, &_frameSynchroStructs[i]._offsrceenRenderFence);
		vkCreateFence(renderContext._device, &fenceCreateInfo, nullptr, &_frameSynchroStructs[i]._defferedRenderFence);
		vkCreateSemaphore(renderContext._device, &semaphoreCreateInfo, nullptr, &_frameSynchroStructs[i]._offsceenRenderSemaphore);
		vkCreateSemaphore(renderContext._device, &semaphoreCreateInfo, nullptr, &_frameSynchroStructs[i]._availabilitySemaphore);
		vkCreateSemaphore(renderContext._device, &semaphoreCreateInfo, nullptr, &_frameSynchroStructs[i]._presentSemaphore);
		vkCreateSemaphore(renderContext._device, &semaphoreCreateInfo, nullptr, &_frameSynchroStructs[i]._defferedRenderSemaphore);
	}
}

void Raytracer::initDataBuffers()
{
	_drawDataIsntanceBuffers.resize(_concurrencyCount);
	_cameraBuffers.resize(_concurrencyCount);
	_globalRenderDataBuffers.resize(_concurrencyCount);
	_drawIndirectCommandBuffer.resize(_concurrencyCount);

	for(unsigned int i = 0; i<_concurrencyCount; i++)
	{
		_drawDataIsntanceBuffers[i] = VGM::Buffer(VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, _maxDrawCount * sizeof(DrawData), renderContext._gpuAllocator);
		_drawIndirectCommandBuffer[i] = VGM::Buffer(VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT, _maxDrawCount * sizeof(VkDrawIndexedIndirectCommand), renderContext._gpuAllocator);
		_cameraBuffers[i] = VGM::Buffer(VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, sizeof(CameraData), renderContext._gpuAllocator);
		_globalRenderDataBuffers[i] = VGM::Buffer(VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, sizeof(globalRenderData), renderContext._gpuAllocator);
	}
}
