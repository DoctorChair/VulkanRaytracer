#include "Raytracer.h"
#include "Extensions.h"
#include "Utils.h"

void Raytracer::init(SDL_Window* window)
{
	int w = 0;
	int h = 0;
	SDL_GetWindowSize(window, &w, &h);

	windowWidth = w;
	windowHeight = h;

	_concurrencyCount = 3;

	VkPhysicalDeviceVulkan13Features features11 = {};
	features11.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_1_FEATURES;

	VkPhysicalDeviceVulkan13Features features12 = {};
	features12.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES;

	VkPhysicalDeviceVulkan13Features features13 = {};
	features13.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES;
	features13.dynamicRendering = VK_TRUE;
	
	VkPhysicalDeviceBufferDeviceAddressFeatures addressFeatures = {};
	addressFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_BUFFER_DEVICE_ADDRESS_FEATURES;
	addressFeatures.bufferDeviceAddress = VK_TRUE;

	VkPhysicalDeviceAccelerationStructureFeaturesKHR accelFeature = {};
	accelFeature.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_ACCELERATION_STRUCTURE_FEATURES_KHR;
	accelFeature.accelerationStructure = VK_TRUE;

	VkPhysicalDeviceRayTracingPipelineFeaturesKHR rtPipelineFeature{};
	rtPipelineFeature.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_TRACING_PIPELINE_FEATURES_KHR;
	rtPipelineFeature.rayTracingPipeline = VK_TRUE;

	VkPhysicalDeviceFeatures2 features2 = {};
	features2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;
	features2.features.multiDrawIndirect = VK_TRUE;
	features2.features.samplerAnisotropy = VK_TRUE;

	features2.pNext = &features13;
	features13.pNext = &addressFeatures;
	addressFeatures.pNext = &accelFeature;
	accelFeature.pNext = &rtPipelineFeature;

	_vulkan = VGM::VulkanContext(window, 1, 3, 0,
		{},
		{ "VK_LAYER_KHRONOS_validation" },
		{ VK_KHR_SWAPCHAIN_EXTENSION_NAME, 
		VK_KHR_ACCELERATION_STRUCTURE_EXTENSION_NAME, 
		VK_KHR_RAY_TRACING_PIPELINE_EXTENSION_NAME, 
		VK_KHR_DEFERRED_HOST_OPERATIONS_EXTENSION_NAME,
		VK_KHR_BUFFER_DEVICE_ADDRESS_EXTENSION_NAME},
		&features2, _concurrencyCount);

	VGM::loadVulkanExtensions(_vulkan._instance, _vulkan._device);

	VkPhysicalDeviceProperties2 properties = { VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2 };
	_raytracingProperties = { VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_TRACING_PIPELINE_PROPERTIES_KHR };
	properties.pNext = &_raytracingProperties;
	vkGetPhysicalDeviceProperties2(_vulkan._physicalDevice, &properties);
	
	initDescriptorSetAllocator();
	initDescriptorSetLayouts();
	initDescripotrSets();
	initGBufferShader();
	initDefferedShader();
	initRaytraceShader();
	initCompositingShader();
	initShaderBindingTable();
	initSyncStructures();
	initCommandBuffers();
	initDataBuffers();
	initLightBuffers();

	initgBuffers();
	initDefferedBuffers();
	initRaytraceBuffers();
	initPresentFramebuffers();
	initMeshBuffer();
	initTextureArrays();
	initTLAS();	
}

Mesh Raytracer::loadMesh(std::vector<Vertex>& vertices, std::vector<uint32_t>& indices, const std::string& name)
{
	auto it = _loadedMeshes.find(name);

	if (it != _loadedMeshes.end())
	{
		return _meshes[it->second];
	}

	Mesh mesh;

	mesh.indicesCount = indices.size();
	mesh.vertexCount = vertices.size();
	mesh.indexOffset = _meshBuffer.indices.size() / sizeof(uint32_t);
	mesh.vertexOffset = _meshBuffer.vertices.size() / sizeof(Vertex);

	VkFence transferFence;
	VkFenceCreateInfo createInfo = {};
	createInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
	createInfo.pNext = nullptr;
	createInfo.flags = 0;

	vkCreateFence(_vulkan._device, &createInfo, nullptr, &transferFence);

	VGM::Buffer vertexTransferBuffer(VK_BUFFER_USAGE_TRANSFER_SRC_BIT, vertices.size() * sizeof(Vertex), _vulkan._generalPurposeAllocator);
	VGM::Buffer indexTransferBuffer(VK_BUFFER_USAGE_TRANSFER_SRC_BIT, indices.size() * sizeof(uint32_t), _vulkan._generalPurposeAllocator);

	vertexTransferBuffer.uploadData(vertices.data(), vertices.size() * sizeof(Vertex), _vulkan._generalPurposeAllocator);
	indexTransferBuffer.uploadData(indices.data(), indices.size() * sizeof(uint32_t), _vulkan._generalPurposeAllocator);

	_transferCommandBuffer = VGM::CommandBuffer(_transferCommandBufferAllocator, _vulkan._device);
	_transferCommandBuffer.begin(nullptr, 0, _vulkan._device);
	
	vertexTransferBuffer.cmdMemoryBarrier(_transferCommandBuffer.get(), VK_ACCESS_HOST_WRITE_BIT, VK_ACCESS_TRANSFER_READ_BIT,
		VK_PIPELINE_STAGE_HOST_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0);
	indexTransferBuffer.cmdMemoryBarrier(_transferCommandBuffer.get(), VK_ACCESS_HOST_WRITE_BIT, VK_ACCESS_TRANSFER_READ_BIT,
		VK_PIPELINE_STAGE_HOST_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0);

		_meshBuffer.vertices.cmdAppendCopyBuffer(_transferCommandBuffer.get(),
		vertexTransferBuffer.get(), vertices.size() * sizeof(Vertex), 0); 
		_meshBuffer.indices.cmdAppendCopyBuffer(_transferCommandBuffer.get(),
		indexTransferBuffer.get(), indices.size() * sizeof(uint32_t), 0);

	_transferCommandBuffer.end();
	VkPipelineStageFlags stageFlags = { VK_PIPELINE_STAGE_NONE };
	VkResult r = _transferCommandBuffer.submit(nullptr, 0, nullptr, 0, &stageFlags, transferFence, _vulkan._transferQeueu);
	
	if (r != VK_SUCCESS)
		throw std::runtime_error("Queue corrupted!\n");

	r = vkWaitForFences(_vulkan._device, 1, &transferFence, VK_TRUE, _timeout);
	r = vkResetFences(_vulkan._device, 1, &transferFence);

	if (r != VK_SUCCESS)
		throw std::runtime_error("Queue corrupted!\n");

	VkDeviceAddress vertexAddress = _meshBuffer.vertices.getDeviceAddress(_vulkan._device);
	VkDeviceAddress indexAddress = _meshBuffer.indices.getDeviceAddress(_vulkan._device);

	_raytraceBuildCommandBuffer.begin(nullptr, 0, _vulkan._device);

	_accelerationStructure.bottomLevelAccelStructures.emplace_back(BLAS(vertexAddress, indexAddress, VK_FORMAT_R32G32B32_SFLOAT, sizeof(Vertex),
		VK_INDEX_TYPE_UINT32, mesh.vertexCount - 1, mesh.vertexOffset, mesh.indicesCount, mesh.indexOffset,
		_vulkan._device, _vulkan._meshAllocator, _raytraceBuildCommandBuffer.get()));

	_raytraceBuildCommandBuffer.end();
	r = _raytraceBuildCommandBuffer.submit(nullptr, 0, nullptr, 0, &stageFlags, transferFence, _vulkan._computeQueue);
	
	if (r != VK_SUCCESS)
		throw std::runtime_error("Queue corrupted!\n");
	
	r = vkWaitForFences(_vulkan._device, 1, &transferFence, VK_TRUE, _timeout);
	r = vkResetFences(_vulkan._device, 1, &transferFence);

	if (r != VK_SUCCESS)
		throw std::runtime_error("Queue corrupted!\n");

	vkDestroyFence(_vulkan._device, transferFence, nullptr);

	mesh.blasIndex = _accelerationStructure.bottomLevelAccelStructures.size() - 1;

	_transferCommandBufferAllocator.reset(_vulkan._device);

	_meshes.push_back(mesh);
	_loadedMeshes.insert(std::make_pair(name, _meshes.size() - 1));

	vertexTransferBuffer.destroy(_vulkan._generalPurposeAllocator);
	indexTransferBuffer.destroy(_vulkan._generalPurposeAllocator);

	return mesh;
}

uint32_t Raytracer::loadTexture(std::vector<unsigned char> pixels, uint32_t width, uint32_t height, uint32_t nrChannels, const std::string& name)
{
	auto it = _loadedImages.find(name);
	if(it != _loadedImages.end())
	{
		return it->second;
	}

	VkFormat format = VK_FORMAT_R8G8B8A8_SRGB;
	switch (nrChannels)
	{
	case 1:
		format = VK_FORMAT_R8_SRGB;
		break;

	case 2:
		format = VK_FORMAT_R8G8_SRGB;
		break;

	case 3:
		format = VK_FORMAT_R8G8B8_SRGB;
		break;

	case 4:
		format = VK_FORMAT_R8G8B8A8_SRGB;
		break;
	case 5:
		return 0;
	}

	VkImageFormatProperties formatProperties;
	vkGetPhysicalDeviceImageFormatProperties(_vulkan._physicalDevice, format, 
		VK_IMAGE_TYPE_2D, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT, 
		0, &formatProperties);

	if (formatProperties.maxExtent.width == 0)
	{
		std::vector<unsigned char> extendendPixels(width * height * 4, 255);
		unsigned int e = 0;
		for(unsigned int i = 0; i < width * height * nrChannels; i = i + nrChannels)
		{
			for(unsigned int j = 0; j < nrChannels; j++)
			{
				extendendPixels[e + j] = pixels[i + j];
			}
			e = e + 4;
		}

		format = VK_FORMAT_R8G8B8A8_SRGB;

		_textures.emplace_back(VGM::Texture(format, VK_IMAGE_TYPE_2D, VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT,
			{ width, height, 1 }, 1, _maxMipMapLevels, _vulkan._textureAllocator));

		pixels = extendendPixels;
	}
	else
	{
		_textures.emplace_back(VGM::Texture(format, VK_IMAGE_TYPE_2D, VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT,
			{ width, height, 1 }, 1, _maxMipMapLevels, _vulkan._textureAllocator));
	}

	
	VkImageView view;
	_textures.back().createImageView(0, _maxMipMapLevels, 0, 1, _vulkan._device, &view);
	_views.push_back(view);

	VkFence transferFence;
	VkFenceCreateInfo createInfo = {};
	createInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
	createInfo.pNext = nullptr;
	createInfo.flags = 0;

	vkCreateFence(_vulkan._device, &createInfo, nullptr, &transferFence);

	VGM::Buffer pixelTransferBuffer(VK_BUFFER_USAGE_TRANSFER_SRC_BIT, pixels.size() * sizeof(unsigned char), _vulkan._generalPurposeAllocator);
	pixelTransferBuffer.uploadData(pixels.data(), pixels.size() * sizeof(unsigned char), _vulkan._generalPurposeAllocator);

	_transferCommandBuffer = VGM::CommandBuffer(_transferCommandBufferAllocator, _vulkan._device);
	_transferCommandBuffer.begin(nullptr, 0, _vulkan._device);

	pixelTransferBuffer.cmdMemoryBarrier(_transferCommandBuffer.get(), VK_ACCESS_HOST_WRITE_BIT, VK_ACCESS_TRANSFER_READ_BIT,
		VK_PIPELINE_STAGE_HOST_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0);

	_textures.back().cmdUploadTextureData(_transferCommandBuffer.get(), 1, 0, pixelTransferBuffer);

	_transferCommandBuffer.end();
	VkPipelineStageFlags stageFlags = { VK_PIPELINE_STAGE_VERTEX_INPUT_BIT };
	_transferCommandBuffer.submit(nullptr, 0, nullptr, 0, &stageFlags, transferFence, _vulkan._transferQeueu);
	vkWaitForFences(_vulkan._device, 1, &transferFence, VK_TRUE, _timeout);
	_transferCommandBufferAllocator.reset(_vulkan._device);
	vkDestroyFence(_vulkan._device, transferFence, nullptr);


	std::vector<VkDescriptorImageInfo> infos(_maxTextureCount);
	for(unsigned int i = 0; i < _maxTextureCount; i++ )
	{
		VkDescriptorImageInfo info;
		info.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

		if (i < _textures.size())
		{
			info.imageView = _views[i];
		}
		else 
		{
			info.imageView = _views.back();
		}
		info.sampler = VK_NULL_HANDLE;

		infos[i] = info;
	}
	
	for (unsigned int i = 0; i < _concurrencyCount; i++)
	{
		VGM::DescriptorSetUpdater::begin(&_textureDescriptorSets[i])
			.bindImage(VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, infos.data(), nullptr, infos.size(), 0, 0)
			.updateDescriptorSet(_vulkan._device);
	}
	


	pixelTransferBuffer.destroy(_vulkan._generalPurposeAllocator);

	_loadedImages.emplace(std::make_pair(name, static_cast<uint32_t>(_textures.size()-1)));
	return _textures.size()-1;
}

void Raytracer::drawMesh(Mesh mesh, glm::mat4 transform, uint32_t objectID)
{
	_drawDataTransferCache.push_back({ transform, mesh.material , objectID });
	_instanceIndexTransferCache.push_back(static_cast<uint32_t>(_instanceIndexTransferCache.size()));
	VkDrawIndexedIndirectCommand command;
	command.vertexOffset = mesh.vertexOffset;
	command.instanceCount = 1;
	command.indexCount = mesh.indicesCount;
	command.firstIndex = mesh.indexOffset;
	command.firstInstance = static_cast<uint32_t>(_drawCommandTransferCache.size());
	_drawCommandTransferCache.push_back(command);
}

void Raytracer::drawSunLight(SunLight light)
{
	_sunLightTransferCache.push_back(light);
}

void Raytracer::drawPointLight(PointLight light)
{
	_pointLightTransferCache.push_back(light);
}

void Raytracer::drawSpotLight(SpotLight light)
{
	_spotLightTransferCache.push_back(light);
}

void Raytracer::setCamera(glm::mat4 viewMatrix, glm::mat4 projectionMatrix, glm::vec3 position)
{
	_cameraData.viewMatrix = viewMatrix;
	_cameraData.projectionMatrix = projectionMatrix;
	_cameraData.cameraPosition = position;
	_cameraData.inverseViewMatrix = glm::inverse(viewMatrix);
	_cameraData.inverseProjectionMatrix = glm::inverse(projectionMatrix);
}

void Raytracer::update()
{
	_currentSwapchainIndex = _vulkan.aquireNextImageIndex(VK_NULL_HANDLE, _frameSynchroStructs[_currentFrameIndex]._presentSemaphore);

	executeDefferedPass();
	executeRaytracePass();
	executeCompositPass();

	_vulkan.presentImage(&_frameSynchroStructs[_currentFrameIndex]._compositingSemaphore, 1);

	_currentFrameIndex = (_currentFrameIndex + 1) % _concurrencyCount;
}

void Raytracer::destroy()
{
	_vulkan.destroy();
}

void Raytracer::initPresentFramebuffers()
{
	_presentFramebuffers.resize(_concurrencyCount);
	VkClearColorValue clearColor = { 1.0f, 0.0f, 0.0f, 0.0f };
	for(unsigned int i = 0; i<_concurrencyCount; i++)
	{
		_presentFramebuffers[i].bindColorTextureTarget(_vulkan._swapchainImageViews[i], VK_ATTACHMENT_LOAD_OP_CLEAR, VK_ATTACHMENT_STORE_OP_STORE, clearColor);
	}
}

void Raytracer::initMeshBuffer()
{
	_meshBuffer.vertices = VGM::Buffer(VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT 
		| VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT | VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR, _maxTriangleCount * 3 * sizeof(Vertex), _vulkan._meshAllocator);
	_meshBuffer.indices = VGM::Buffer(VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT 
		| VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT | VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR, _maxTriangleCount * sizeof(uint32_t), _vulkan._meshAllocator);
}

void Raytracer::initTextureArrays()
{
	_textures.reserve(_maxTextureCount);
	_views.reserve(_maxTextureCount);
}

void Raytracer::initShaderBindingTable()
{
	VkPhysicalDeviceRayTracingPipelinePropertiesKHR pipelineProperties = {};
	pipelineProperties.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_TRACING_PIPELINE_PROPERTIES_KHR;
	pipelineProperties.pNext = nullptr;
	VGM::getPhysicalDeviceProperties2(_vulkan._physicalDevice, &pipelineProperties);

	uint32_t groupAlignment = pipelineProperties.shaderGroupHandleAlignment;
	uint32_t groupHandleSize = pipelineProperties.shaderGroupHandleSize;
	uint32_t groupBaseAlignment = pipelineProperties.shaderGroupBaseAlignment;

	std::vector<uint8_t> shaderGroupHandels;
	_raytraceShader.getShaderHandles(shaderGroupHandels, _vulkan._device, groupHandleSize, groupAlignment);

	_shaderBindingTable = ShaderBindingTable(_raytraceShader.shaderGroupCount(), groupHandleSize, groupAlignment, shaderGroupHandels,
		_vulkan._device, _vulkan._generalPurposeAllocator, groupBaseAlignment, _raytraceShader.missShaderCount(), _raytraceShader.hitShaderCount());
}

void Raytracer::loadDummyMeshInstance()
{
	Vertex vertex0, vertex1, vertex2;
	vertex0.position = { -1.0f, -1.0f, 0.0f };
	vertex1.position = { -0.0f, -1.0f, 0.0f };
	vertex2.position = { 0.0f, 1.0f, 0.0f };
	std::vector<Vertex> vertices = {vertex0, vertex1, vertex2};
	std::vector<uint32_t> indices = { 0, 1, 2 };
	std::vector<unsigned char> pixels(128 * 128 * 4, 0);
	uint32_t index = loadTexture(pixels, 128, 128, 4, "dummy");

	_dummyMesh = loadMesh(vertices, indices, "dummy");
	_dummyMesh.material.albedoIndex = index;
	_dummyMesh.material.normalIndex = index;
	_dummyMesh.material.metallicIndex = index;
	_dummyMesh.material.roughnessIndex = index;
}

void Raytracer::initTLAS()
{
	loadDummyMeshInstance();

	VGM::CommandBuffer raytraceBuildCommandBuffer(_computeCommandBufferAllocator, _vulkan._device);

	_accelerationStructure.instanceBuffer = VGM::Buffer(VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT 
		| VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR,
		_maxDrawCount * sizeof(VkAccelerationStructureInstanceKHR), _vulkan._generalPurposeAllocator);

	VkAccelerationStructureInstanceKHR dummyInstance;
	dummyInstance.transform.matrix[0][0] = 1.0f;
	dummyInstance.transform.matrix[1][1] = 1.0f;
	dummyInstance.transform.matrix[2][2] = 1.0f;
	dummyInstance.transform.matrix[2][3] = 1.0f;
	dummyInstance.accelerationStructureReference = _accelerationStructure.bottomLevelAccelStructures.back().getAddress(_vulkan._device);
	dummyInstance.flags = VK_GEOMETRY_INSTANCE_TRIANGLE_FACING_CULL_DISABLE_BIT_KHR;
	dummyInstance.instanceCustomIndex = 0;
	dummyInstance.mask = 0xFF;
	dummyInstance.instanceShaderBindingTableRecordOffset = 0;

	void* dst = _accelerationStructure.instanceBuffer.map(_vulkan._generalPurposeAllocator);
	memcpy(dst, &dummyInstance, sizeof(VkAccelerationStructureInstanceKHR));
	_accelerationStructure.instanceBuffer.unmap(_vulkan._generalPurposeAllocator);

	raytraceBuildCommandBuffer.begin(nullptr, 0, _vulkan._device);

	_accelerationStructure.instanceBuffer.cmdMemoryBarrier(raytraceBuildCommandBuffer.get(), VK_ACCESS_TRANSFER_WRITE_BIT,
		VK_ACCESS_ACCELERATION_STRUCTURE_WRITE_BIT_KHR,
		VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_ACCELERATION_STRUCTURE_BUILD_BIT_KHR, 0);

	uint32_t numInstances = 1;
	VkDeviceAddress instanceAddress = _accelerationStructure.instanceBuffer.getDeviceAddress(_vulkan._device);
	
	//error in this function 
	_accelerationStructure.topLevelAccelStructure = TLAS(_accelerationStructure.instanceBuffer.get(), 1,
		&instanceAddress, 1, &numInstances, 1, _vulkan._device, _vulkan._meshAllocator, raytraceBuildCommandBuffer.get());

	raytraceBuildCommandBuffer.end();
	
	raytraceBuildCommandBuffer.submit(nullptr, 0, nullptr, 0, nullptr, VK_NULL_HANDLE, _vulkan._computeQueue);
}

void Raytracer::updateGBufferDescriptorSets()
{
	VGM::Buffer* currentGlobalRenderDataBuffer = &_globalRenderDataBuffers[_currentFrameIndex];
	VGM::Buffer* currentCameraBuffer = &_cameraBuffers[_currentFrameIndex];
	VGM::Buffer* currentDrawDataInstanceBuffer = &_drawDataIsntanceBuffers[_currentFrameIndex];
	VGM::Buffer* currentDrawIndirectBuffer = &_drawIndirectCommandBuffer[_currentFrameIndex];
	GBuffer* currentGBuffer = &_gBufferChain[_currentFrameIndex];


	VkDescriptorBufferInfo globalbufferInfo;
	globalbufferInfo.buffer = currentGlobalRenderDataBuffer->get();
	globalbufferInfo.offset = 0;
	globalbufferInfo.range = currentGlobalRenderDataBuffer->size();

	VkDescriptorBufferInfo camerabufferInfo;
	camerabufferInfo.buffer = currentCameraBuffer->get();
	camerabufferInfo.offset = 0;
	camerabufferInfo.range = currentCameraBuffer->size();

	VkDescriptorImageInfo albedoInfo = {};
	albedoInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	albedoInfo.sampler = _gBufferShader.getSamplers()[0];

	VkDescriptorImageInfo metallicInfo = {};
	metallicInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	metallicInfo.sampler = _gBufferShader.getSamplers()[1];

	VkDescriptorImageInfo normalInfo = {};
	normalInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	normalInfo.sampler = _gBufferShader.getSamplers()[2];

	VkDescriptorImageInfo roughnessInfo = {};
	roughnessInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	roughnessInfo.sampler = _gBufferShader.getSamplers()[3];

	VkDescriptorBufferInfo drawDataInstanceInfo;
	drawDataInstanceInfo.buffer = currentDrawDataInstanceBuffer->get();
	drawDataInstanceInfo.offset = 0;
	drawDataInstanceInfo.range = currentDrawDataInstanceBuffer->size();

	VkDescriptorSet sets[] = { _globalDescriptorSets[_currentFrameIndex], _gBuffer1DescripotrSets[_currentFrameIndex], _gBuffer2DescriptorSets[_currentFrameIndex]};

	VGM::DescriptorSetUpdater::begin(sets)
		.bindBuffer(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, camerabufferInfo, 1, 0, 0)
		.bindBuffer(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, globalbufferInfo, 1, 0, 1)
		.bindImage(VK_DESCRIPTOR_TYPE_SAMPLER, &albedoInfo, nullptr, 1, 1, 0)
		.bindImage(VK_DESCRIPTOR_TYPE_SAMPLER, &metallicInfo, nullptr, 1, 1, 1)
		.bindImage(VK_DESCRIPTOR_TYPE_SAMPLER, &normalInfo, nullptr, 1, 1, 2)
		.bindImage(VK_DESCRIPTOR_TYPE_SAMPLER, &roughnessInfo, nullptr, 1, 1, 3)
		.bindBuffer(VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, drawDataInstanceInfo, 1, 2, 0)
		.updateDescriptorSet(_vulkan._device);
}

void Raytracer::updateDefferedDescriptorSets()
{
	GBuffer* currentGBuffer = &_gBufferChain[_currentFrameIndex];
	
	VGM::Buffer* currentSunLightBuffer = &_sunLightBuffers[_currentFrameIndex];
	VGM::Buffer* currentPointLightBuffer = &_pointLightBuffers[_currentFrameIndex];
	VGM::Buffer* currentSpotLightBuffer = &_spotLightBuffers[_currentFrameIndex];

	VkDescriptorImageInfo albedoInfo;
	albedoInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	albedoInfo.imageView = currentGBuffer->colorView;
	albedoInfo.sampler = _defferedShader.getSamplers()[0];

	VkDescriptorImageInfo normalInfo;
	normalInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	normalInfo.imageView = currentGBuffer->normalView;
	normalInfo.sampler = _defferedShader.getSamplers()[1];

	VkDescriptorImageInfo depthInfo;
	depthInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	depthInfo.imageView = currentGBuffer->depthView;
	depthInfo.sampler = _defferedShader.getSamplers()[2];

	VkDescriptorImageInfo roughnessMetalnessInfo;
	roughnessMetalnessInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	roughnessMetalnessInfo.imageView = currentGBuffer->roughnessMetalnessView;
	roughnessMetalnessInfo.sampler = _defferedShader.getSamplers()[3];

	VkDescriptorImageInfo idInfo;
	idInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	idInfo.imageView = currentGBuffer->idView;
	idInfo.sampler = _defferedShader.getSamplers()[4];

	VkDescriptorBufferInfo sunInfo = {};
	sunInfo.buffer = currentSunLightBuffer->get();
	sunInfo.offset = 0;
	sunInfo.range = currentSunLightBuffer->size();

	VkDescriptorBufferInfo pointInfo = {};
	pointInfo.buffer = currentPointLightBuffer->get();
	pointInfo.offset = 0;
	pointInfo.range = currentPointLightBuffer->size();

	VkDescriptorBufferInfo spotInfo = {};
	spotInfo.buffer = currentSpotLightBuffer->get();
	spotInfo.offset = 0;
	spotInfo.range = currentSpotLightBuffer->size();

	VkDescriptorSet sets[] = { _defferedDescriptorSets[_currentFrameIndex], _lightDescripotrSets[_currentFrameIndex] };

	VGM::DescriptorSetUpdater::begin(sets)
		.bindImage(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, &albedoInfo, nullptr, 1, 0, 0)
		.bindImage(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, &normalInfo, nullptr, 1, 0, 1)
		.bindImage(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, &depthInfo, nullptr, 1, 0, 2)
		.bindImage(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, &roughnessMetalnessInfo, nullptr, 1, 0, 3)
		.bindImage(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, &idInfo, nullptr, 1, 0, 4)
		.bindBuffer(VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, sunInfo, 1, 1, 0)
		.bindBuffer(VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, pointInfo, 1, 1, 1)
		.bindBuffer(VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, spotInfo, 1, 1, 2)
		.updateDescriptorSet(_vulkan._device);	
}

void Raytracer::updateRaytraceDescripotrSets()
{
	GBuffer* currentGBuffer = &_gBufferChain[_currentFrameIndex];
	DefferedBuffer* currentDefferedBuffer = &_defferdBufferChain[_currentFrameIndex];

	
	VkDescriptorImageInfo colorInfo;
	colorInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	colorInfo.imageView = currentDefferedBuffer->colorView;
	colorInfo.sampler = _raytraceShader.getSamplers()[0];

	VkDescriptorImageInfo normalInfo;
	normalInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	normalInfo.imageView = currentGBuffer->normalView;
	normalInfo.sampler = _raytraceShader.getSamplers()[1];

	VkDescriptorImageInfo depthInfo;
	depthInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	depthInfo.imageView = currentGBuffer->depthView;
	depthInfo.sampler = _raytraceShader.getSamplers()[2];

	VkDescriptorImageInfo outColorInfo;
	outColorInfo.imageLayout = VK_IMAGE_LAYOUT_GENERAL;
	outColorInfo.imageView = _raytraceBufferChain[_currentFrameIndex].colorView;
	outColorInfo.sampler = VK_NULL_HANDLE;

	VkWriteDescriptorSetAccelerationStructureKHR accelWrite;
	accelWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET_ACCELERATION_STRUCTURE_KHR;
	accelWrite.pNext = nullptr;
	accelWrite.pAccelerationStructures = _accelerationStructure.topLevelAccelStructure.get();
	accelWrite.accelerationStructureCount = 1;

	VkDescriptorSet sets[] = { _raytracer1DescriptorSets[_currentFrameIndex], _raytracer2DescriptorSets[_currentFrameIndex] };
	VGM::DescriptorSetUpdater::begin(sets)
		.bindImage(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, &colorInfo, nullptr, 1, 0, 0)
		.bindImage(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, &normalInfo, nullptr, 1, 0, 1)
		.bindImage(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, &depthInfo, nullptr, 1, 0, 2)
		.bindAccelerationStructure(VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR, &accelWrite, 1, 1, 0)
		.bindImage(VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, &outColorInfo, nullptr, 1, 1, 1)
		.updateDescriptorSet(_vulkan._device);
}

void Raytracer::updateCompositingDescriptorSets()
{
	VkDescriptorImageInfo outColorInfo;
	outColorInfo.imageLayout = VK_IMAGE_LAYOUT_GENERAL;
	outColorInfo.imageView = _raytraceBufferChain[_currentFrameIndex].colorView;
	outColorInfo.sampler = VK_NULL_HANDLE;

	VkDescriptorSet sets[] = { _compositingDescriptorSets[_currentFrameIndex] };
	VGM::DescriptorSetUpdater::begin(sets)
		.bindImage(VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, &outColorInfo, nullptr, 1, 0, 0)
	.updateDescriptorSet(_vulkan._device);
}

void Raytracer::executeDefferedPass()
{
	GBuffer* currentGBuffer = &_gBufferChain[_currentFrameIndex];
	DefferedBuffer* currentDefferedBuffer = &_defferdBufferChain[_currentFrameIndex];
	VkFence defferedRenderFence = _frameSynchroStructs[_currentFrameIndex]._defferedRenderFence;
	VkFence raytraceRenderFence = _frameSynchroStructs[_currentFrameIndex]._raytraceFence;
	VkSemaphore presentSemaphore = _frameSynchroStructs[_currentFrameIndex]._presentSemaphore;
	VGM::Buffer* currentDrawDataInstanceBuffer = &_drawDataIsntanceBuffers[_currentFrameIndex];
	VGM::Buffer* currentDrawIndirectBuffer = &_drawIndirectCommandBuffer[_currentFrameIndex];
	VGM::DescriptorSetAllocator* currentDefferedDescriptorSetAllocator = &_defferedDescriptorSetAllocator;

	VGM::CommandBuffer* defferedCmd = &_defferedRenderCommandBuffers[_currentFrameIndex];

	VkRect2D renderArea = { 0, 0, windowWidth, windowHeight };
	
	VkFence fences[] = { defferedRenderFence, raytraceRenderFence };

	defferedCmd->begin(fences, std::size(fences), _vulkan._device);

	_drawIndirectCommandBuffer[_currentFrameIndex].uploadData(_drawCommandTransferCache.data(),
		_drawCommandTransferCache.size() * sizeof(VkDrawIndexedIndirectCommand),
		_vulkan._generalPurposeAllocator);
	_drawDataIsntanceBuffers[_currentFrameIndex].uploadData(_drawDataTransferCache.data(),
		_drawDataTransferCache.size() * sizeof(DrawData),
		_vulkan._generalPurposeAllocator);

	_instanceIndexTransferBuffers[_currentFrameIndex].uploadData(_instanceIndexTransferCache.data(),
		_instanceIndexTransferCache.size() * sizeof(uint32_t),
		_vulkan._generalPurposeAllocator);

	_sunLightBuffers[_currentFrameIndex].uploadData(_sunLightTransferCache.data(),
		_sunLightTransferCache.size() * sizeof(SunLight), _vulkan._generalPurposeAllocator);
	_pointLightBuffers[_currentFrameIndex].uploadData(_pointLightTransferCache.data(),
		_pointLightTransferCache.size() * sizeof(PointLight), _vulkan._generalPurposeAllocator);
	_spotLightBuffers[_currentFrameIndex].uploadData(_spotLightTransferCache.data(),
		_spotLightTransferCache.size() * sizeof(SpotLight), _vulkan._generalPurposeAllocator);

	_globalRenderData.sunLightCount = static_cast<uint32_t>(_sunLightTransferCache.size());
	_globalRenderData.pointLightCout = static_cast<uint32_t>(_pointLightTransferCache.size());
	_globalRenderData.spotLightCount = static_cast<uint32_t>(_spotLightTransferCache.size());
	
	_cameraBuffers[_currentFrameIndex].uploadData(&_cameraData, sizeof(CameraData), _vulkan._generalPurposeAllocator);
	_globalRenderDataBuffers[_currentFrameIndex].uploadData(&_globalRenderData, sizeof(GlobalRenderData), _vulkan._generalPurposeAllocator);

	updateGBufferDescriptorSets();

	updateDefferedDescriptorSets();

	_drawIndirectCommandBuffer[_currentFrameIndex].cmdMemoryBarrier(defferedCmd->get(), VK_ACCESS_HOST_WRITE_BIT, VK_ACCESS_SHADER_READ_BIT,
		VK_PIPELINE_STAGE_HOST_BIT, VK_PIPELINE_STAGE_VERTEX_SHADER_BIT, 0);
	_drawDataIsntanceBuffers[_currentFrameIndex].cmdMemoryBarrier(defferedCmd->get(), VK_ACCESS_HOST_WRITE_BIT, VK_ACCESS_SHADER_READ_BIT,
		VK_PIPELINE_STAGE_HOST_BIT, VK_PIPELINE_STAGE_VERTEX_SHADER_BIT, 0);
	_instanceIndexTransferBuffers[_currentFrameIndex].cmdMemoryBarrier(defferedCmd->get(), VK_ACCESS_HOST_WRITE_BIT, VK_ACCESS_SHADER_READ_BIT,
		VK_PIPELINE_STAGE_HOST_BIT, VK_PIPELINE_STAGE_VERTEX_SHADER_BIT, 0);

	_sunLightBuffers[_currentFrameIndex].cmdMemoryBarrier(defferedCmd->get(),
		VK_ACCESS_HOST_WRITE_BIT, VK_ACCESS_SHADER_READ_BIT, VK_PIPELINE_STAGE_HOST_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0);
	_pointLightBuffers[_currentFrameIndex].cmdMemoryBarrier(defferedCmd->get(),
		VK_ACCESS_HOST_WRITE_BIT, VK_ACCESS_SHADER_READ_BIT, VK_PIPELINE_STAGE_HOST_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0);
	_pointLightBuffers[_currentFrameIndex].cmdMemoryBarrier(defferedCmd->get(),
		VK_ACCESS_HOST_WRITE_BIT, VK_ACCESS_SHADER_READ_BIT, VK_PIPELINE_STAGE_HOST_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0);

	_cameraBuffers[_currentFrameIndex].cmdMemoryBarrier(defferedCmd->get(), VK_ACCESS_HOST_WRITE_BIT, VK_ACCESS_SHADER_READ_BIT,
		VK_PIPELINE_STAGE_HOST_BIT, VK_PIPELINE_STAGE_VERTEX_SHADER_BIT, 0);
	_globalRenderDataBuffers[_currentFrameIndex].cmdMemoryBarrier(defferedCmd->get(), VK_ACCESS_HOST_WRITE_BIT, VK_ACCESS_SHADER_READ_BIT,
		VK_PIPELINE_STAGE_HOST_BIT, VK_PIPELINE_STAGE_VERTEX_SHADER_BIT, 0);

	VkImageSubresourceRange subresource;
	subresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	subresource.baseArrayLayer = 0;
	subresource.baseMipLevel = 0;
	subresource.layerCount = 1;
	subresource.levelCount = 1;

	currentGBuffer->colorBuffer.cmdTransitionLayout(defferedCmd->get(), subresource, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
		VK_ACCESS_NONE, VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT);

	currentGBuffer->roughnessMetalnessBuffer.cmdTransitionLayout(defferedCmd->get(), subresource, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
		VK_ACCESS_NONE, VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT);

	currentGBuffer->idBuffer.cmdTransitionLayout(defferedCmd->get(), subresource, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
		VK_ACCESS_NONE, VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT);

	currentGBuffer->normalBuffer.cmdTransitionLayout(defferedCmd->get(), subresource, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
		VK_ACCESS_NONE, VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT);

	subresource.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
	currentGBuffer->depthBuffer.cmdTransitionLayout(defferedCmd->get(), subresource, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL,
		VK_ACCESS_NONE, VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT,
		VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT);

	VkDescriptorSet offscrenSets[] = { _globalDescriptorSets[_currentFrameIndex],
		_gBuffer1DescripotrSets[_currentFrameIndex],
		_gBuffer2DescriptorSets[_currentFrameIndex],
		_textureDescriptorSets[_currentFrameIndex] };

	currentGBuffer->framebuffer.cmdBeginRendering(defferedCmd->get(), renderArea);

	_gBufferShader.cmdBind(defferedCmd->get());
	vkCmdBindDescriptorSets(defferedCmd->get(), VK_PIPELINE_BIND_POINT_GRAPHICS, _gBufferPipelineLayout, 0, std::size(offscrenSets), offscrenSets, 0, nullptr);
	vkCmdBindIndexBuffer(defferedCmd->get(), _meshBuffer.indices.get(), 0, VK_INDEX_TYPE_UINT32);
	VkBuffer vertexBuffers[] = { _meshBuffer.vertices.get() };
	VkDeviceSize offsets[] = { 0 };
	vkCmdBindVertexBuffers(defferedCmd->get(), 0, std::size(vertexBuffers), vertexBuffers, offsets);

	vkCmdDrawIndexedIndirect(defferedCmd->get(), currentDrawIndirectBuffer->get(), 0, (uint32_t)_drawCommandTransferCache.size(),
		sizeof(VkDrawIndexedIndirectCommand));

	currentGBuffer->framebuffer.cmdEndRendering(defferedCmd->get());

	subresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	currentGBuffer->colorBuffer.cmdTransitionLayout(defferedCmd->get(), subresource,
		VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
		VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT, VK_ACCESS_SHADER_READ_BIT, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, 
		VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT);

	currentGBuffer->roughnessMetalnessBuffer.cmdTransitionLayout(defferedCmd->get(), subresource,
		VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
		VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT, VK_ACCESS_SHADER_READ_BIT, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
		VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT);

	currentGBuffer->idBuffer.cmdTransitionLayout(defferedCmd->get(), subresource,
		VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
		VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT, VK_ACCESS_SHADER_READ_BIT, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, 
		VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT);

	currentGBuffer->normalBuffer.cmdTransitionLayout(defferedCmd->get(), subresource,
		VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
		VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT, VK_ACCESS_SHADER_READ_BIT, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, 
		VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT);

	subresource.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
	currentGBuffer->depthBuffer.cmdTransitionLayout(defferedCmd->get(), subresource, VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
		VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT, VK_ACCESS_SHADER_READ_BIT,
		VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT, 
		VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT);

	subresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	currentDefferedBuffer->colorBuffer.cmdTransitionLayout(defferedCmd->get(), subresource, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
		VK_ACCESS_NONE, VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT);

	VkDescriptorSet defferedSets[] = {
		_globalDescriptorSets[_currentFrameIndex],
		_defferedDescriptorSets[_currentFrameIndex],
		_lightDescripotrSets[_currentFrameIndex] };

	currentDefferedBuffer->framebuffer.cmdBeginRendering(defferedCmd->get(), renderArea);

	_defferedShader.cmdBind(defferedCmd->get());
	vkCmdBindDescriptorSets(defferedCmd->get(), VK_PIPELINE_BIND_POINT_GRAPHICS, _defferedPipelineLayout, 0, std::size(defferedSets), defferedSets, 0, nullptr);

	vkCmdDraw(defferedCmd->get(), 3, 1, 0, 0);

	currentDefferedBuffer->framebuffer.cmdEndRendering(defferedCmd->get());

	subresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	currentGBuffer->colorBuffer.cmdTransitionLayout(defferedCmd->get(), subresource,
		VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
		VK_ACCESS_SHADER_READ_BIT, VK_ACCESS_SHADER_READ_BIT, 
		VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
		VK_PIPELINE_STAGE_RAY_TRACING_SHADER_BIT_KHR);

	currentGBuffer->normalBuffer.cmdTransitionLayout(defferedCmd->get(), subresource,
		VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
		VK_ACCESS_SHADER_READ_BIT, VK_ACCESS_SHADER_READ_BIT, 
		VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
		VK_PIPELINE_STAGE_RAY_TRACING_SHADER_BIT_KHR);

	subresource.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
	currentGBuffer->depthBuffer.cmdTransitionLayout(defferedCmd->get(), subresource, 
		VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
		VK_ACCESS_SHADER_READ_BIT, VK_ACCESS_SHADER_READ_BIT,
		VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
		VK_PIPELINE_STAGE_RAY_TRACING_SHADER_BIT_KHR);

	subresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	currentDefferedBuffer->colorBuffer.cmdTransitionLayout(defferedCmd->get(), subresource, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
		VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT, VK_ACCESS_SHADER_READ_BIT, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
		VK_PIPELINE_STAGE_RAY_TRACING_SHADER_BIT_KHR);

	defferedCmd->end();

	VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
	defferedCmd->submit(nullptr, 0, &presentSemaphore, 1, waitStages, defferedRenderFence, _vulkan._graphicsQeueu);

	_sunLightTransferCache.clear();
	_pointLightTransferCache.clear();
	_spotLightTransferCache.clear();
	_drawCommandTransferCache.clear();
	_drawDataTransferCache.clear();
	_instanceIndexTransferCache.clear();
}

void Raytracer::executeRaytracePass()
{
	VGM::CommandBuffer* raytraceCmd = &_raytraceRenderCommandBuffers[_currentFrameIndex];
	VkFence raytraceFence = _frameSynchroStructs[_currentFrameIndex]._raytraceFence;
	VkSemaphore raytraceSemaphore = _frameSynchroStructs[_currentFrameIndex]._raytraceSemaphore;
	VkSemaphore defferedSemaphore = _frameSynchroStructs[_currentFrameIndex]._defferedRenderSemaphore;

	GBuffer* currentGBuffer = &_gBufferChain[_currentFrameIndex];
	DefferedBuffer* currentDefferedBuffer = &_defferdBufferChain[_currentFrameIndex];
	RaytraceBuffer* currentRaytraceBuffer = &_raytraceBufferChain[_currentFrameIndex];

	VkDescriptorSet sets[] = {
		_globalDescriptorSets[_currentFrameIndex],
		_raytracer1DescriptorSets[_currentFrameIndex],
		_raytracer2DescriptorSets[_currentFrameIndex]
	};

	raytraceCmd->begin(nullptr, 0, _vulkan._device);

	updateRaytraceDescripotrSets();

	VkImageSubresourceRange subresource;
	subresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	subresource.baseArrayLayer = 0;
	subresource.baseMipLevel = 0;
	subresource.layerCount = 1;
	subresource.levelCount = 1;

	currentRaytraceBuffer->colorBuffer.cmdTransitionLayout(raytraceCmd->get(), subresource,
		VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_GENERAL, VK_ACCESS_NONE, VK_ACCESS_SHADER_WRITE_BIT,
		VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_RAY_TRACING_SHADER_BIT_KHR);

	_raytraceShader.cmdBind(raytraceCmd->get());
	vkCmdBindDescriptorSets(raytraceCmd->get(), VK_PIPELINE_BIND_POINT_RAY_TRACING_KHR, _raytracePipelineLayout, 0, 3, sets, 0, nullptr);

	VkStridedDeviceAddressRegionKHR callableRegion = {};
	VkStridedDeviceAddressRegionKHR raygenRegion = _shaderBindingTable.getRaygenRegion();
	VkStridedDeviceAddressRegionKHR hitRegion = _shaderBindingTable.getHitRegion();
	VkStridedDeviceAddressRegionKHR missRegion = _shaderBindingTable.getMissRegion();

	vkCmdTraceRaysKHR(raytraceCmd->get(),
		&raygenRegion, &missRegion,
		&hitRegion, &callableRegion,
		windowWidth, windowHeight, 1);

	currentRaytraceBuffer->colorBuffer.cmdTransitionLayout(raytraceCmd->get(), subresource,
		VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_LAYOUT_GENERAL, VK_ACCESS_SHADER_WRITE_BIT, VK_ACCESS_SHADER_READ_BIT,
		VK_PIPELINE_STAGE_RAY_TRACING_SHADER_BIT_KHR, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT);

	raytraceCmd->end();

	raytraceCmd->submit(nullptr, 0, nullptr, 0, nullptr, raytraceFence, _vulkan._computeQueue);
}

void Raytracer::executeCompositPass()
{
	RaytraceBuffer* currentRaytraceBuffer = &_raytraceBufferChain[_currentFrameIndex];

	VGM::CommandBuffer* compositingCmd = &_compositingCommandBuffers[_currentFrameIndex];
	VkDescriptorSet currentCompositingDescriptorSet = _compositingDescriptorSets[_currentFrameIndex];

	VkFence currentFence = _frameSynchroStructs[_currentFrameIndex]._compositingFence;
	VkSemaphore currentCompositSemaphore = _frameSynchroStructs[_currentFrameIndex]._compositingSemaphore;

	VGM::Framebuffer* currentPresentFramebuffer = &_presentFramebuffers[_currentSwapchainIndex];


	VkDescriptorSet sets[] = {
		currentCompositingDescriptorSet
	};

	VkRect2D renderArea = { 0, 0, windowWidth, windowHeight };

	compositingCmd->begin(&currentFence, 1, _vulkan._device);

	updateCompositingDescriptorSets();

	VkImageSubresourceRange subresource;
	subresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	subresource.baseArrayLayer = 0;
	subresource.baseMipLevel = 0;
	subresource.layerCount = 1;
	subresource.levelCount = 1;

	_vulkan.cmdPrepareSwapchainImageForRendering(compositingCmd->get());
	currentPresentFramebuffer->cmdBeginRendering(compositingCmd->get(), renderArea);

	_compositingShader.cmdBind(compositingCmd->get());
	vkCmdBindDescriptorSets(compositingCmd->get(), VK_PIPELINE_BIND_POINT_GRAPHICS, _compositingPipelineLayout, 0, std::size(sets), sets, 0, nullptr);

	vkCmdDraw(compositingCmd->get(), 3, 1, 0, 0);

	currentPresentFramebuffer->cmdEndRendering(compositingCmd->get());
	_vulkan.cmdPrepareSwaphainImageForPresent(compositingCmd->get());

	compositingCmd->end();
	VkPipelineStageFlags waitFlags = { VK_PIPELINE_STAGE_RAY_TRACING_SHADER_BIT_KHR };
	compositingCmd->submit(&currentCompositSemaphore, 1, nullptr, 0, &waitFlags, currentFence, _vulkan._graphicsQeueu);
}

void Raytracer::initgBuffers()
{
	_gBufferChain.resize(_concurrencyCount);
	for (auto& g : _gBufferChain)
	{
		g.normalBuffer = VGM::Texture(VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_TYPE_2D, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
			{ windowWidth, windowHeight, 1 }, 1, 1, _vulkan._gpuAllocator);
		g.idBuffer =  VGM::Texture(VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_TYPE_2D, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
			{ windowWidth, windowHeight, 1 }, 1, 1, _vulkan._gpuAllocator);
		g.colorBuffer = VGM::Texture(VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_TYPE_2D, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
			{ windowWidth, windowHeight, 1 }, 1, 1, _vulkan._gpuAllocator);
		g.roughnessMetalnessBuffer = VGM::Texture(VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_TYPE_2D, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
			{ windowWidth, windowHeight, 1 }, 1, 1, _vulkan._gpuAllocator);
		g.depthBuffer = VGM::Texture(VK_FORMAT_D32_SFLOAT, VK_IMAGE_TYPE_2D, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
			{ windowWidth, windowHeight, 1 }, 1, 1, _vulkan._gpuAllocator);

		VkClearColorValue clear = { 1.0f, 1.0f, 1.0f };

		g.colorBuffer.createImageView(0, 1, 0, 1, _vulkan._device, &g.colorView);
		g.framebuffer.bindColorTextureTarget(g.colorView, VK_ATTACHMENT_LOAD_OP_CLEAR, VK_ATTACHMENT_STORE_OP_STORE, clear);

		g.normalBuffer.createImageView(0, 1, 0, 1, _vulkan._device, &g.normalView);
		g.framebuffer.bindColorTextureTarget(g.normalView, VK_ATTACHMENT_LOAD_OP_CLEAR, VK_ATTACHMENT_STORE_OP_STORE, clear);

		g.roughnessMetalnessBuffer.createImageView(0, 1, 0, 1, _vulkan._device, &g.roughnessMetalnessView);
		g.framebuffer.bindColorTextureTarget(g.roughnessMetalnessView, VK_ATTACHMENT_LOAD_OP_CLEAR, VK_ATTACHMENT_STORE_OP_STORE, clear);

		g.idBuffer.createImageView(0, 1, 0, 1, _vulkan._device, &g.idView);
		g.framebuffer.bindColorTextureTarget(g.idView, VK_ATTACHMENT_LOAD_OP_CLEAR, VK_ATTACHMENT_STORE_OP_STORE, clear);

		g.depthBuffer.createImageView(0, 1, 0, 1, _vulkan._device, &g.depthView);
		g.framebuffer.bindDepthTextureTarget(g.depthView, VK_ATTACHMENT_LOAD_OP_CLEAR, VK_ATTACHMENT_STORE_OP_STORE, 1.0f);
	}
}

void Raytracer::initDefferedBuffers()
{
	_defferdBufferChain.resize(_concurrencyCount);
	for(auto& d : _defferdBufferChain)
	{
		d.colorBuffer = VGM::Texture(VK_FORMAT_R32G32B32A32_SFLOAT, VK_IMAGE_TYPE_2D, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
			{ windowWidth, windowHeight, 1 }, 1, 1, _vulkan._gpuAllocator);
		d.colorBuffer.createImageView(0, 1, 0, 1, _vulkan._device, &d.colorView);

		VkClearColorValue clear = { 0.0f, 0.0f, 1.0f };

		d.framebuffer.bindColorTextureTarget(d.colorView, VK_ATTACHMENT_LOAD_OP_CLEAR, VK_ATTACHMENT_STORE_OP_STORE, clear);
	}
}

void Raytracer::initRaytraceBuffers()
{
	_raytraceBufferChain.resize(_concurrencyCount);
	for (auto& r : _raytraceBufferChain)
	{
		r.colorBuffer = VGM::Texture(VK_FORMAT_R32G32B32A32_SFLOAT, VK_IMAGE_TYPE_2D, VK_IMAGE_USAGE_STORAGE_BIT,
			{ windowWidth, windowHeight, 1 }, 1, 1, _vulkan._gpuAllocator);
		r.colorBuffer.createImageView(0, 1, 0, 1, _vulkan._device, &r.colorView);
	}
}

void Raytracer::initDescriptorSetAllocator()
{
	std::vector<VkDescriptorPoolSize> poolSizes = {
	{VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 10},
	{VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 10},
	{VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 10},
	{VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, _maxTextureCount},
	{VK_DESCRIPTOR_TYPE_SAMPLER, 10},
	{VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR, 10 }
	};
	
	_offsecreenDescriptorSetAllocator = VGM::DescriptorSetAllocator(poolSizes, 100, 0, 10, _vulkan._device);
	_defferedDescriptorSetAllocator = VGM::DescriptorSetAllocator(poolSizes, 100, 0, 10, _vulkan._device);
	_textureDescriptorSetAllocator = VGM::DescriptorSetAllocator(poolSizes, 100, 0, 10, _vulkan._device);
	_raytracerDescriptorSetAllocator = VGM::DescriptorSetAllocator(poolSizes, 100, 0, 10, _vulkan._device);
	_compositingDescriptorSetAllocator = VGM::DescriptorSetAllocator(poolSizes, 100, 0, 10, _vulkan._device);
}

void Raytracer::initDescriptorSetLayouts()
{
	VGM::DescriptorSetLayoutBuilder::begin()
		.addBinding(0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, nullptr, VK_SHADER_STAGE_ALL_GRAPHICS | VK_SHADER_STAGE_RAYGEN_BIT_KHR, 1)
		.addBinding(1, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, nullptr, VK_SHADER_STAGE_ALL_GRAPHICS | VK_SHADER_STAGE_RAYGEN_BIT_KHR, 1)
		.createDescriptorSetLayout(_vulkan._device, &_globalLayout);

	VGM::DescriptorSetLayoutBuilder::begin()
		.addBinding(0, VK_DESCRIPTOR_TYPE_SAMPLER, nullptr, VK_SHADER_STAGE_ALL_GRAPHICS, 1)
		.addBinding(1, VK_DESCRIPTOR_TYPE_SAMPLER, nullptr, VK_SHADER_STAGE_ALL_GRAPHICS, 1)
		.addBinding(2, VK_DESCRIPTOR_TYPE_SAMPLER, nullptr, VK_SHADER_STAGE_ALL_GRAPHICS, 1)
		.addBinding(3, VK_DESCRIPTOR_TYPE_SAMPLER, nullptr, VK_SHADER_STAGE_ALL_GRAPHICS, 1)
		.createDescriptorSetLayout(_vulkan._device, &_gBufferLayout1);

	VGM::DescriptorSetLayoutBuilder::begin()
		.addBinding(0, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, nullptr, VK_SHADER_STAGE_ALL_GRAPHICS, 1)
		.createDescriptorSetLayout(_vulkan._device, &_gBufferLayout2);

	VGM::DescriptorSetLayoutBuilder::begin()
		.addBinding(0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, nullptr, VK_SHADER_STAGE_ALL_GRAPHICS, 1)
		.addBinding(1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, nullptr, VK_SHADER_STAGE_ALL_GRAPHICS, 1)
		.addBinding(2, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, nullptr, VK_SHADER_STAGE_ALL_GRAPHICS, 1)
		.addBinding(3, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, nullptr, VK_SHADER_STAGE_ALL_GRAPHICS, 1)
		.addBinding(4, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, nullptr, VK_SHADER_STAGE_ALL_GRAPHICS, 1)
		.createDescriptorSetLayout(_vulkan._device, &_defferedLayout);

	VGM::DescriptorSetLayoutBuilder::begin()
		.addBinding(0, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, nullptr, VK_SHADER_STAGE_ALL_GRAPHICS, 1)
		.addBinding(1, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, nullptr, VK_SHADER_STAGE_ALL_GRAPHICS, 1)
		.addBinding(2, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, nullptr, VK_SHADER_STAGE_ALL_GRAPHICS, 1)
		.createDescriptorSetLayout(_vulkan._device, &_lightLayout);

	VGM::DescriptorSetLayoutBuilder::begin()
		.addBinding(0, VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, nullptr, VK_SHADER_STAGE_ALL_GRAPHICS, _maxTextureCount)
		.createDescriptorSetLayout(_vulkan._device, &_textureLayout);

	VGM::DescriptorSetLayoutBuilder::begin()
		.addBinding(0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, nullptr, VK_SHADER_STAGE_RAYGEN_BIT_KHR, 1)
		.addBinding(1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, nullptr, VK_SHADER_STAGE_RAYGEN_BIT_KHR, 1)
		.addBinding(2, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, nullptr, VK_SHADER_STAGE_RAYGEN_BIT_KHR, 1)
		.createDescriptorSetLayout(_vulkan._device, &_raytracerLayout1);

	VGM::DescriptorSetLayoutBuilder::begin()
		.addBinding(0, VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR, nullptr, VK_SHADER_STAGE_RAYGEN_BIT_KHR, 1)
		.addBinding(1, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, nullptr, VK_SHADER_STAGE_RAYGEN_BIT_KHR, 1)
		.createDescriptorSetLayout(_vulkan._device, &_raytracerLayout2);

	VGM::DescriptorSetLayoutBuilder::begin()
		.addBinding(0, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, nullptr, VK_SHADER_STAGE_ALL_GRAPHICS, 1)
		.createDescriptorSetLayout(_vulkan._device, &_compositingLayout);
}

void Raytracer::initDescripotrSets()
{
	_globalDescriptorSets.resize(_concurrencyCount);
	_gBuffer1DescripotrSets.resize(_concurrencyCount);
	_gBuffer2DescriptorSets.resize(_concurrencyCount);
	_defferedDescriptorSets.resize(_concurrencyCount);
	_lightDescripotrSets.resize(_concurrencyCount);
	_textureDescriptorSets.resize(_concurrencyCount);
	_raytracer1DescriptorSets.resize(_concurrencyCount);
	_raytracer2DescriptorSets.resize(_concurrencyCount);
	_compositingDescriptorSets.resize(_concurrencyCount);

	for(unsigned int i = 0; i< _concurrencyCount; i++)
	{
		VGM::DescriptorSetBuilder::begin()

			.createDescriptorSet(_vulkan._device, &_globalDescriptorSets[i], &_globalLayout, _offsecreenDescriptorSetAllocator);
		VGM::DescriptorSetBuilder::begin()

			.createDescriptorSet(_vulkan._device, &_gBuffer1DescripotrSets[i], &_gBufferLayout1, _offsecreenDescriptorSetAllocator);
		VGM::DescriptorSetBuilder::begin()
			.createDescriptorSet(_vulkan._device, &_gBuffer2DescriptorSets[i], &_gBufferLayout2, _offsecreenDescriptorSetAllocator);

		VGM::DescriptorSetBuilder::begin()
			.createDescriptorSet(_vulkan._device, &_defferedDescriptorSets[i], &_defferedLayout, _defferedDescriptorSetAllocator);

		VGM::DescriptorSetBuilder::begin()
			.createDescriptorSet(_vulkan._device, &_lightDescripotrSets[i], &_lightLayout, _defferedDescriptorSetAllocator);

		VGM::DescriptorSetBuilder::begin()
			.createDescriptorSet(_vulkan._device, &_textureDescriptorSets[i], &_textureLayout, _defferedDescriptorSetAllocator);

		VGM::DescriptorSetBuilder::begin()
			.createDescriptorSet(_vulkan._device, &_raytracer1DescriptorSets[i], &_raytracerLayout1, _raytracerDescriptorSetAllocator);
		VGM::DescriptorSetBuilder::begin()
			.createDescriptorSet(_vulkan._device, &_raytracer2DescriptorSets[i], &_raytracerLayout2, _raytracerDescriptorSetAllocator);

		VGM::DescriptorSetBuilder::begin()
			.createDescriptorSet(_vulkan._device, &_compositingDescriptorSets[i], &_compositingLayout, _compositingDescriptorSetAllocator);
	}
}

void Raytracer::initGBufferShader()
{
	VkDescriptorSetLayout setLayouts[] = { _globalLayout, _gBufferLayout1, _gBufferLayout2, _textureLayout};

	VkPipelineLayoutCreateInfo createInfo = {};
	createInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	createInfo.pNext = nullptr;
	createInfo.flags = 0;
	createInfo.pushConstantRangeCount = 0;
	createInfo.setLayoutCount = 4;
	createInfo.pSetLayouts = setLayouts;

	vkCreatePipelineLayout(_vulkan._device, &createInfo, nullptr, &_gBufferPipelineLayout);

	std::vector<std::pair<std::string, VkShaderStageFlagBits>> sources =
	{
		{"C:\\Users\\Eric\\projects\\VulkanRaytracing\\shaders\\SPIRV\\gBufferVERT.spv", VK_SHADER_STAGE_VERTEX_BIT},
		{"C:\\Users\\Eric\\projects\\VulkanRaytracing\\shaders\\SPIRV\\gBufferFRAG.spv", VK_SHADER_STAGE_FRAGMENT_BIT}
	};

	VGM::PipelineConfigurator configurator;
	std::vector<VkFormat> renderingColorFormats = { VK_FORMAT_R8G8B8A8_SRGB, VK_FORMAT_R8G8B8A8_SRGB, VK_FORMAT_R8G8B8A8_SRGB, VK_FORMAT_R8G8B8A8_SRGB };

	VkPipelineColorBlendAttachmentState colorBlendAttachment = {};
	colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
	colorBlendAttachment.blendEnable = VK_TRUE;
	colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
	colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
	colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD;
	colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
	colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
	colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD;
	std::vector<VkPipelineColorBlendAttachmentState> colorBlendAttachments = { colorBlendAttachment, colorBlendAttachment, colorBlendAttachment, colorBlendAttachment };

	configurator.setRenderingFormats(renderingColorFormats, VK_FORMAT_D32_SFLOAT, VK_FORMAT_UNDEFINED);
	configurator.setViewportState(windowWidth, windowHeight, 0.0f, 1.0f, 0.0f, 0.0f);
	configurator.setScissorState(windowWidth, windowHeight, 0.0f, 0.0f);
	configurator.setInputAssemblyState(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST);
	configurator.setBackfaceCulling(VK_CULL_MODE_BACK_BIT);
	configurator.setColorBlendingState(colorBlendAttachments);
	configurator.setDepthState(VK_TRUE, VK_TRUE, VK_COMPARE_OP_LESS_OR_EQUAL);
	

	configurator.addVertexAttribInputDescription(0, 0, VK_FORMAT_R32G32B32_SFLOAT, 0); //Position
	configurator.addVertexAttribInputDescription(1, 0, VK_FORMAT_R32G32_SFLOAT, 3 * sizeof(float)); //texCoord
	configurator.addVertexAttribInputDescription(2, 0, VK_FORMAT_R32G32_SFLOAT, 5 * sizeof(float)); //texCoord
	configurator.addVertexAttribInputDescription(3, 0, VK_FORMAT_R32G32_SFLOAT, 7 * sizeof(float)); //texCoord
	configurator.addVertexAttribInputDescription(4, 0, VK_FORMAT_R32G32_SFLOAT, 9 * sizeof(float)); //texCoord
	configurator.addVertexAttribInputDescription(5, 0, VK_FORMAT_R32G32B32_SFLOAT, 11 * sizeof(float)); //normal
	configurator.addVertexAttribInputDescription(6, 0, VK_FORMAT_R32G32B32_SFLOAT, 14 * sizeof(float)); //tangent
	configurator.addVertexAttribInputDescription(7, 0, VK_FORMAT_R32G32B32_SFLOAT, 17 * sizeof(float)); //bitangant
	configurator.addVertexInputInputBindingDescription(0, sizeof(Vertex), VK_VERTEX_INPUT_RATE_VERTEX);

	uint32_t numSamplers = 10;

	_gBufferShader = VGM::ShaderProgram(sources, _gBufferPipelineLayout, configurator, _vulkan._device, numSamplers);
}

void Raytracer::initDefferedShader()
{
	VkDescriptorSetLayout setLayouts[] = {_globalLayout, _defferedLayout, _lightLayout};

	VkPipelineLayoutCreateInfo createInfo = {};
	createInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	createInfo.pNext = nullptr;
	createInfo.flags = 0;
	createInfo.pushConstantRangeCount = 0;
	createInfo.setLayoutCount = 3;
	createInfo.pSetLayouts = setLayouts;
	
	vkCreatePipelineLayout(_vulkan._device, &createInfo, nullptr, &_defferedPipelineLayout);

	std::vector<std::pair<std::string, VkShaderStageFlagBits>> sources =
	{
		{"C:\\Users\\Eric\\projects\\VulkanRaytracing\\shaders\\SPIRV\\defferedVERT.spv", VK_SHADER_STAGE_VERTEX_BIT},
		{"C:\\Users\\Eric\\projects\\VulkanRaytracing\\shaders\\SPIRV\\defferedFRAG.spv", VK_SHADER_STAGE_FRAGMENT_BIT}
	};

	VGM::PipelineConfigurator configurator;
	std::vector<VkFormat> renderingColorFormats = { VK_FORMAT_R32G32B32A32_SFLOAT };

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

	uint32_t numSamplers = 10;

	_defferedShader = VGM::ShaderProgram(sources, _defferedPipelineLayout, configurator, _vulkan._device, numSamplers);
}

void Raytracer::initRaytraceShader()
{
	VkDescriptorSetLayout layouts[] = { _globalLayout ,_raytracerLayout1, _raytracerLayout2 };

	std::vector<std::pair<std::string, VkShaderStageFlagBits>> sources = {
		{"C:\\Users\\Eric\\projects\\VulkanRaytracing\\shaders\\SPIRV\\raytraceRGEN.spv", VK_SHADER_STAGE_RAYGEN_BIT_KHR},
		{"C:\\Users\\Eric\\projects\\VulkanRaytracing\\shaders\\SPIRV\\raytraceRMISS.spv", VK_SHADER_STAGE_MISS_BIT_KHR},
		{"C:\\Users\\Eric\\projects\\VulkanRaytracing\\shaders\\SPIRV\\raytraceRCHIT.spv", VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR} };

	VkPipelineLayoutCreateInfo createInfo = {};
	createInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	createInfo.pNext = nullptr;
	createInfo.flags = 0;
	createInfo.pushConstantRangeCount = 0;
	createInfo.setLayoutCount = 3;
	createInfo.pSetLayouts = layouts;

	vkCreatePipelineLayout(_vulkan._device, &createInfo, nullptr, &_raytracePipelineLayout);
	
	_raytraceShader = RaytracingShader(sources, _raytracePipelineLayout, _vulkan._device, 1, 4);
}

void Raytracer::initCompositingShader()
{
	VkDescriptorSetLayout setLayouts[] = { _compositingLayout };

	VkPipelineLayoutCreateInfo createInfo = {};
	createInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	createInfo.pNext = nullptr;
	createInfo.flags = 0;
	createInfo.pushConstantRangeCount = 0;
	createInfo.setLayoutCount = 1;
	createInfo.pSetLayouts = setLayouts;

	vkCreatePipelineLayout(_vulkan._device, &createInfo, nullptr, &_compositingPipelineLayout);

	std::vector<std::pair<std::string, VkShaderStageFlagBits>> sources =
	{
		{"C:\\Users\\Eric\\projects\\VulkanRaytracing\\shaders\\SPIRV\\compositVERT.spv", VK_SHADER_STAGE_VERTEX_BIT},
		{"C:\\Users\\Eric\\projects\\VulkanRaytracing\\shaders\\SPIRV\\compositFRAG.spv", VK_SHADER_STAGE_FRAGMENT_BIT}
	};

	VGM::PipelineConfigurator configurator;
	std::vector<VkFormat> renderingColorFormats = { _vulkan._swapchainFormat.format };

	VkPipelineColorBlendAttachmentState colorBlendAttachment = {};
	colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
	colorBlendAttachment.blendEnable = VK_FALSE;
	std::vector<VkPipelineColorBlendAttachmentState> colorBlendAttachments = { colorBlendAttachment };

	configurator.setRenderingFormats(renderingColorFormats, VK_FORMAT_UNDEFINED, VK_FORMAT_UNDEFINED);
	configurator.setViewportState(windowWidth, windowHeight, 0.0f, 1.0f, 0.0f, 0.0f);
	configurator.setScissorState(windowWidth, windowHeight, 0.0f, 0.0f);
	configurator.setInputAssemblyState(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST);
	configurator.setBackfaceCulling(VK_CULL_MODE_NONE);
	configurator.setColorBlendingState(colorBlendAttachments);

	_compositingShader = VGM::ShaderProgram(sources, _compositingPipelineLayout, configurator, _vulkan._device, 1);
}

void Raytracer::initCommandBuffers()
{
	_renderCommandBufferAllocator = VGM::CommandBufferAllocator(_vulkan._graphicsQueueFamilyIndex, _vulkan._device);
	
	_computeCommandBufferAllocator = VGM::CommandBufferAllocator(_vulkan._computeQueueFamilyIndex, _vulkan._device);

	_offsceenRenderCommandBuffers.resize(_concurrencyCount);
	for(auto& c : _offsceenRenderCommandBuffers)
	{
		c = VGM::CommandBuffer(_renderCommandBufferAllocator, _vulkan._device);
	}

	_defferedRenderCommandBuffers.resize(_concurrencyCount);
	for (auto& c : _defferedRenderCommandBuffers)
	{
		c = VGM::CommandBuffer(_renderCommandBufferAllocator, _vulkan._device);
	}

	_raytraceRenderCommandBuffers.resize(_concurrencyCount);
	for(auto& c : _raytraceRenderCommandBuffers)
	{
		c = VGM::CommandBuffer(_computeCommandBufferAllocator, _vulkan._device);
	}

	_compositingCommandBuffers.resize(_concurrencyCount);
	for (auto& c : _compositingCommandBuffers)
	{
		c = VGM::CommandBuffer(_computeCommandBufferAllocator, _vulkan._device);
	}

	_transferCommandBufferAllocator = VGM::CommandBufferAllocator(_vulkan._transferQueueFamilyIndex, _vulkan._device);

	_raytraceBuildCommandBuffer = VGM::CommandBuffer(_computeCommandBufferAllocator, _vulkan._device);
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

	VkEventCreateInfo eventCreateInfo = {};
	eventCreateInfo.sType = VK_STRUCTURE_TYPE_EVENT_CREATE_INFO;
	eventCreateInfo.pNext = nullptr;
	eventCreateInfo.flags = 0;


	_frameSynchroStructs.resize(_concurrencyCount);
	for (unsigned int i = 0; i < _concurrencyCount; i++)
	{
		vkCreateFence(_vulkan._device, &fenceCreateInfo, nullptr, &_frameSynchroStructs[i]._offsrceenRenderFence);
		vkCreateFence(_vulkan._device, &fenceCreateInfo, nullptr, &_frameSynchroStructs[i]._defferedRenderFence);
		vkCreateFence(_vulkan._device, &fenceCreateInfo, nullptr, &_frameSynchroStructs[i]._raytraceFence);
		vkCreateFence(_vulkan._device, &fenceCreateInfo, nullptr, &_frameSynchroStructs[i]._compositingFence);
		vkCreateSemaphore(_vulkan._device, &semaphoreCreateInfo, nullptr, &_frameSynchroStructs[i]._offsceenRenderSemaphore);
		vkCreateSemaphore(_vulkan._device, &semaphoreCreateInfo, nullptr, &_frameSynchroStructs[i]._availabilitySemaphore);
		vkCreateSemaphore(_vulkan._device, &semaphoreCreateInfo, nullptr, &_frameSynchroStructs[i]._presentSemaphore);
		vkCreateSemaphore(_vulkan._device, &semaphoreCreateInfo, nullptr, &_frameSynchroStructs[i]._defferedRenderSemaphore);
		vkCreateSemaphore(_vulkan._device, &semaphoreCreateInfo, nullptr, &_frameSynchroStructs[i]._compositingSemaphore);
		vkCreateSemaphore(_vulkan._device, &semaphoreCreateInfo, nullptr, &_frameSynchroStructs[i]._raytraceSemaphore);

		vkCreateEvent(_vulkan._device, &eventCreateInfo, nullptr, &_frameSynchroStructs[i]._defferedFinishedEvent);
		vkCreateEvent(_vulkan._device, &eventCreateInfo, nullptr, &_frameSynchroStructs[i]._raytraceFinishedEvent);
	}
}

void Raytracer::initDataBuffers()
{
	_drawDataIsntanceBuffers.resize(_concurrencyCount);
	_cameraBuffers.resize(_concurrencyCount);
	_globalRenderDataBuffers.resize(_concurrencyCount);
	_drawIndirectCommandBuffer.resize(_concurrencyCount);
	_instanceIndexBuffers.resize(_concurrencyCount);
	_instanceIndexTransferBuffers.resize(_concurrencyCount);

	for(unsigned int i = 0; i<_concurrencyCount; i++)
	{
		_drawDataIsntanceBuffers[i] = VGM::Buffer(VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, _maxDrawCount * sizeof(DrawData), _vulkan._generalPurposeAllocator);
		_drawIndirectCommandBuffer[i] = VGM::Buffer(VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT, _maxDrawCount * sizeof(VkDrawIndexedIndirectCommand), _vulkan._generalPurposeAllocator);
		_cameraBuffers[i] = VGM::Buffer(VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, sizeof(CameraData), _vulkan._generalPurposeAllocator);
		_globalRenderDataBuffers[i] = VGM::Buffer(VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, sizeof(GlobalRenderData), _vulkan._generalPurposeAllocator);
		_instanceIndexBuffers[i] = VGM::Buffer(VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, _maxDrawCount * sizeof(uint32_t), _vulkan._generalPurposeAllocator);
		_instanceIndexTransferBuffers[i] = VGM::Buffer(VK_BUFFER_USAGE_TRANSFER_SRC_BIT, _maxDrawCount * sizeof(uint32_t), _vulkan._generalPurposeAllocator);
	}

	
}

void Raytracer::initLightBuffers()
{
	_sunLightBuffers.resize(_concurrencyCount);
	_pointLightBuffers.resize(_concurrencyCount);
	_spotLightBuffers.resize(_concurrencyCount);
	for(unsigned int i = 0; i<_concurrencyCount; i++)
	{
		_pointLightBuffers[i] = VGM::Buffer(VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, _maxPointLighst * sizeof(PointLight), _vulkan._generalPurposeAllocator);
		_sunLightBuffers[i] = VGM::Buffer(VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, _maxPointLighst * sizeof(SunLight), _vulkan._generalPurposeAllocator);
		_spotLightBuffers[i] = VGM::Buffer(VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, _maxPointLighst * sizeof(SpotLight), _vulkan._generalPurposeAllocator);
	}
}
