#include "Raytracer.h"
#include "Extensions.h"
#include "Utils.h"

#include <glm/ext/matrix_transform.hpp>

void Raytracer::init(SDL_Window* window, uint32_t windowWidth, uint32_t windowHeight)
{
	VkPhysicalDeviceVulkan13Features features11 = {};
	features11.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_1_FEATURES;

	VkPhysicalDeviceVulkan13Features features12 = {};
	features12.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES;

	VkPhysicalDeviceVulkan13Features features13 = {};
	features13.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES;
	features13.dynamicRendering = VK_TRUE;
	features13.synchronization2 = VK_TRUE;

	VkPhysicalDeviceBufferDeviceAddressFeatures addressFeatures = {};
	addressFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_BUFFER_DEVICE_ADDRESS_FEATURES;
	addressFeatures.bufferDeviceAddress = VK_TRUE;
	addressFeatures.bufferDeviceAddressCaptureReplay = VK_TRUE;

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
	features2.features.independentBlend = VK_TRUE;

	features2.pNext = &features13;
	features13.pNext = &addressFeatures;
	addressFeatures.pNext = &accelFeature;
	accelFeature.pNext = &rtPipelineFeature;

	int w = 0, h = 0;
	SDL_GetWindowSize(window, &w, &h);

	_vulkan = VGM::VulkanContext(window, w, h, 
		1, 3, 0,
		{},
		{ "VK_LAYER_KHRONOS_validation" },
		{ VK_KHR_SWAPCHAIN_EXTENSION_NAME, 
		VK_KHR_ACCELERATION_STRUCTURE_EXTENSION_NAME, 
		VK_KHR_RAY_TRACING_PIPELINE_EXTENSION_NAME, 
		VK_KHR_DEFERRED_HOST_OPERATIONS_EXTENSION_NAME,
		VK_KHR_BUFFER_DEVICE_ADDRESS_EXTENSION_NAME, "VK_EXT_depth_range_unrestricted"},
		&features2, 3);

	_windowWidth = w;
	_windowHeight = h;

	VGM::loadVulkanExtensions(_vulkan._instance, _vulkan._device);

	VkPhysicalDeviceProperties2 properties = { VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2 };
	_raytracingProperties = { VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_TRACING_PIPELINE_PROPERTIES_KHR };
	properties.pNext = &_raytracingProperties;
	vkGetPhysicalDeviceProperties2(_vulkan._physicalDevice, &properties);
	
	initDescriptorSetAllocator();
	initDescriptorSetLayouts();
	initDescripotrSets();
	initGBufferShader();
	initRaytraceShader();
	initPostProcessingChain();
	initShaderBindingTable();
	initSyncStructures();
	initCommandBuffers();
	initDataBuffers();
	initLightBuffers();

	genHaltonSequence(2, 3, _sampleSequenceLength);

	initHistoryBuffers();
	initBufferState();

	initgBuffers();
	initRaytraceBuffers();
	initPostProcessBuffers();
	initMeshBuffer();
	initTextureArrays();
	loadPlaceholderMeshAndTexture();	
	initSamplers();
	updateDescriptorSets();
}

void Raytracer::resizeSwapchain(uint32_t windowWidth, uint32_t windowHeight)
{
	_vulkan.recreateSwapchain(windowWidth, windowHeight);
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

	VGM::Buffer vertexTransferBuffer(VK_BUFFER_USAGE_TRANSFER_SRC_BIT, vertices.size() * sizeof(Vertex), _vulkan._generalPurposeAllocator, _vulkan._device);
	VGM::Buffer indexTransferBuffer(VK_BUFFER_USAGE_TRANSFER_SRC_BIT, indices.size() * sizeof(uint32_t), _vulkan._generalPurposeAllocator, _vulkan._device);

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

	VkDeviceAddress vertexAddress = _meshBuffer.vertices.getDeviceAddress(_vulkan._device);
	VkDeviceAddress indexAddress = _meshBuffer.indices.getDeviceAddress(_vulkan._device);

	_meshBuffer.vertices.cmdMemoryBarrier(_transferCommandBuffer.get(), VK_ACCESS_TRANSFER_WRITE_BIT, 
		VK_ACCESS_ACCELERATION_STRUCTURE_READ_BIT_KHR | VK_ACCESS_ACCELERATION_STRUCTURE_WRITE_BIT_KHR,
		VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_ACCELERATION_STRUCTURE_BUILD_BIT_KHR, 0);
	_meshBuffer.indices.cmdMemoryBarrier(_transferCommandBuffer.get(), VK_ACCESS_TRANSFER_WRITE_BIT, 
		VK_ACCESS_ACCELERATION_STRUCTURE_READ_BIT_KHR | VK_ACCESS_ACCELERATION_STRUCTURE_WRITE_BIT_KHR,
		VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_ACCELERATION_STRUCTURE_BUILD_BIT_KHR, 0);

	VkPhysicalDeviceAccelerationStructurePropertiesKHR accelProperties = {};
	accelProperties.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_ACCELERATION_STRUCTURE_PROPERTIES_KHR;
	VkPhysicalDeviceProperties2 features = {};
	features.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2;
	features.pNext = &accelProperties;

	vkGetPhysicalDeviceProperties2(_vulkan._physicalDevice, &features);

	_accelerationStructure.bottomLevelAccelStructures.emplace_back(BLAS(vertexAddress, indexAddress, VK_FORMAT_R32G32B32_SFLOAT, accelProperties.minAccelerationStructureScratchOffsetAlignment, 
		sizeof(Vertex), VK_INDEX_TYPE_UINT32, mesh.vertexCount - 1, mesh.vertexOffset, mesh.indicesCount, mesh.indexOffset,
		_vulkan._device, _vulkan._meshAllocator, _transferCommandBuffer.get()));

	_meshBuffer.vertices.cmdMemoryBarrier(_transferCommandBuffer.get(), 
		VK_ACCESS_ACCELERATION_STRUCTURE_READ_BIT_KHR | VK_ACCESS_ACCELERATION_STRUCTURE_WRITE_BIT_KHR,
		VK_ACCESS_ACCELERATION_STRUCTURE_READ_BIT_KHR,
		VK_PIPELINE_STAGE_ACCELERATION_STRUCTURE_BUILD_BIT_KHR, VK_PIPELINE_STAGE_RAY_TRACING_SHADER_BIT_KHR, 0);
	_meshBuffer.indices.cmdMemoryBarrier(_transferCommandBuffer.get(), 
		VK_ACCESS_ACCELERATION_STRUCTURE_READ_BIT_KHR | VK_ACCESS_ACCELERATION_STRUCTURE_WRITE_BIT_KHR,
		VK_ACCESS_ACCELERATION_STRUCTURE_READ_BIT_KHR,
		VK_PIPELINE_STAGE_ACCELERATION_STRUCTURE_BUILD_BIT_KHR, VK_PIPELINE_STAGE_RAY_TRACING_SHADER_BIT_KHR, 0);

	_transferCommandBuffer.end();

	VkPipelineStageFlags stageFlags = { VK_PIPELINE_STAGE_NONE };
	VK_CHECK(_transferCommandBuffer.submit(nullptr, 0, nullptr, 0, &stageFlags, transferFence, _vulkan._computeQueue));
	VK_CHECK(vkWaitForFences(_vulkan._device, 1, &transferFence, VK_TRUE, _timeout));
	VK_CHECK(vkResetFences(_vulkan._device, 1, &transferFence));

	vkDestroyFence(_vulkan._device, transferFence, nullptr);

	mesh.blasIndex = _accelerationStructure.bottomLevelAccelStructures.size() - 1;

	_transferCommandBufferAllocator.reset(_vulkan._device);

	_meshes.push_back(mesh);
	_loadedMeshes.insert(std::make_pair(name, _meshes.size() - 1));

	vertexTransferBuffer.destroy(_vulkan._generalPurposeAllocator);
	indexTransferBuffer.destroy(_vulkan._generalPurposeAllocator);


	return mesh;
}

MeshInstance Raytracer::getMeshInstance(const std::string& name)
{
	MeshInstance instance;
	uint32_t index = _loadedMeshes.at(name);
	instance.meshIndex = index;
	instance.blasIndex = _meshes[index].blasIndex;
	instance.instanceID = _activeInstanceCount;

	_activeInstanceCount++;

	return instance;
}

uint32_t Raytracer::loadTexture(std::vector<unsigned char>& pixels, uint32_t width, uint32_t height, uint32_t nrChannels, const std::string& name)
{
	auto it = _loadedImages.find(name);
	if(it != _loadedImages.end())
	{
		return it->second;
	}

	VkFormat format = VK_FORMAT_R8G8B8A8_UNORM;
	switch (nrChannels)
	{
	case 1:
		format = VK_FORMAT_R8_UNORM;
		break;

	case 2:
		format = VK_FORMAT_R8G8_UNORM;
		break;

	case 3:
		format = VK_FORMAT_R8G8B8_UNORM;
		break;

	case 4:
		format = VK_FORMAT_R8G8B8A8_UNORM;
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

		format = VK_FORMAT_R8G8B8A8_UNORM;

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

	VGM::Buffer pixelTransferBuffer(VK_BUFFER_USAGE_TRANSFER_SRC_BIT, pixels.size() * sizeof(unsigned char), _vulkan._generalPurposeAllocator, _vulkan._device);
	pixelTransferBuffer.uploadData(pixels.data(), pixels.size() * sizeof(unsigned char), _vulkan._generalPurposeAllocator);

	_transferCommandBuffer = VGM::CommandBuffer(_transferCommandBufferAllocator, _vulkan._device);
	_transferCommandBuffer.begin(nullptr, 0, _vulkan._device);

	pixelTransferBuffer.cmdMemoryBarrier(_transferCommandBuffer.get(), VK_ACCESS_HOST_WRITE_BIT, VK_ACCESS_TRANSFER_READ_BIT,
		VK_PIPELINE_STAGE_HOST_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0);

	_textures.back().cmdUploadTextureData(_transferCommandBuffer.get(), 1, 0, pixelTransferBuffer);

	VkImageSubresourceRange subresource = {};
	subresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	subresource.baseArrayLayer = 0;
	subresource.baseMipLevel = 0;
	subresource.layerCount = 1;
	subresource.levelCount = _textures.back().mipMapLevels();

	_textures.back().cmdTransitionLayout(_transferCommandBuffer.get(), subresource, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, 
		VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, 
		VK_ACCESS_SHADER_READ_BIT, VK_ACCESS_SHADER_READ_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 
		VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT | VK_PIPELINE_STAGE_RAY_TRACING_SHADER_BIT_KHR);

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

void Raytracer::drawMeshInstance(MeshInstance meshInstance, uint32_t cullMask)
{
	Mesh mesh = _meshes[meshInstance.meshIndex];
	Material material = meshInstance.material;
	BLAS* blas = &_accelerationStructure.bottomLevelAccelStructures[meshInstance.blasIndex];
	
	VkDrawIndexedIndirectCommand command;
	command.vertexOffset = mesh.vertexOffset;
	command.instanceCount = 1;
	command.indexCount = mesh.indicesCount;
	command.firstIndex = mesh.indexOffset;
	command.firstInstance = static_cast<uint32_t>(_drawCommandTransferCache.size());
	
	_drawCommandTransferCache.push_back(command);
	_drawDataTransferCache.push_back({ meshInstance.transform, meshInstance.previousTransform, material , meshInstance.instanceID, mesh.vertexOffset, mesh.indexOffset});

	glm::mat4 transform = glm::transpose(meshInstance.transform);

	VkAccelerationStructureInstanceKHR instace = {};
	instace.transform.matrix[0][0] = transform[0][0];
	instace.transform.matrix[0][1] = transform[0][1];
	instace.transform.matrix[0][2] = transform[0][2];
	instace.transform.matrix[0][3] = transform[0][3];

	instace.transform.matrix[1][0] = transform[1][0];
	instace.transform.matrix[1][1] = transform[1][1];
	instace.transform.matrix[1][2] = transform[1][2];
	instace.transform.matrix[1][3] = transform[1][3];

	instace.transform.matrix[2][0] = transform[2][0];
	instace.transform.matrix[2][1] = transform[2][1];
	instace.transform.matrix[2][2] = transform[1][1];
	instace.transform.matrix[2][3] = transform[2][3];

	instace.accelerationStructureReference = _accelerationStructure.bottomLevelAccelStructures[meshInstance.blasIndex].getAddress(_vulkan._device);
	instace.flags = VK_GEOMETRY_INSTANCE_TRIANGLE_FACING_CULL_DISABLE_BIT_KHR;
	instace.instanceCustomIndex = command.firstInstance;
	instace.mask = cullMask;
	instace.instanceShaderBindingTableRecordOffset = 0;
	
	_accelerationStructure._instanceTransferCache.push_back(instace);
}

void Raytracer::drawMesh(Mesh mesh, glm::mat4 transform, uint32_t objectID)
{
	/*_drawDataTransferCache.push_back({ transform, mesh.material , objectID });
	
	VkDrawIndexedIndirectCommand command;
	command.vertexOffset = mesh.vertexOffset;
	command.instanceCount = 1;
	command.indexCount = mesh.indicesCount;
	command.firstIndex = mesh.indexOffset;
	command.firstInstance = static_cast<uint32_t>(_drawCommandTransferCache.size());
	_drawCommandTransferCache.push_back(command);*/
}

void Raytracer::drawSunLight(SunLight light)
{
	_sunLightTransferCache.push_back(light);
}

void Raytracer::drawPointLight(PointLightSourceInstance light)
{
	PointLight p;
	p.position = light.position;
	p.radius = light.radius;
	p.emissionIndex = light.lightModel.material.emissionIndex;
	p.strength = light.strength;

	glm::mat4 transform = glm::translate(glm::mat4(1.0f), light.position) * glm::scale(glm::mat4(1.0f), glm::vec3(light.radius * 2.0f));

	light.lightModel.transform = transform;
	_pointLightTransferCache.push_back(p);
	drawMeshInstance(light.lightModel, 0x10);
}

void Raytracer::drawSpotLight(SpotLight light)
{
	_spotLightTransferCache.push_back(light);
}

void Raytracer::setNoiseTextureIndex(uint32_t index)
{
	_globalRenderData.noiseSampleTextureIndex = index;
}

void Raytracer::setCamera(glm::mat4 viewMatrix, glm::mat4 projectionMatrix, glm::vec3 position)
{
	_cameraData.previousViewMatrix = _cameraData.viewMatrix;
	_cameraData.previousProjectionMatrix = _cameraData.projectionMatrix;

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
	executePostProcessPass();

	_vulkan.presentImage(&_frameSynchroStructs[_currentFrameIndex]._postProcessSemaphore, 1);

	_currentFrameIndex = (_currentFrameIndex + 1) % _concurrencyCount;

	_historyBuffer.currentIndex = (_historyBuffer.currentIndex + 1) % _historyLength;

	_frameNumber++;
}	

void Raytracer::destroy()
{
	_vulkan.destroy();
}

void Raytracer::initMeshBuffer()
{
	_meshBuffer.vertices = VGM::Buffer(VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT
		| VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT | VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR,
		VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT, _maxTriangleCount * 3 * sizeof(Vertex), _vulkan._meshAllocator, _vulkan._device);
	
	_meshBuffer.indices = VGM::Buffer(VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT
		| VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT | VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR,
		VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT, _maxTriangleCount * sizeof(uint32_t), _vulkan._meshAllocator, _vulkan._device);
	
	_meshBuffer.addressBuffer = VGM::Buffer(VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
		sizeof(MeshBufferAddressData), _vulkan._meshAllocator, _vulkan._device);

	MeshBufferAddressData addressData = { _meshBuffer.vertices.getDeviceAddress(_vulkan._device), _meshBuffer.indices.getDeviceAddress(_vulkan._device)};

	uint8_t* ptr = reinterpret_cast<uint8_t*>(_meshBuffer.addressBuffer.map(_vulkan._meshAllocator));
	_meshBuffer.addressBuffer.memcopy(ptr, &addressData, sizeof(addressData));
	_meshBuffer.addressBuffer.unmap(_vulkan._meshAllocator);
}

void Raytracer::initTextureArrays()
{
	_textures.reserve(_maxTextureCount);
	_views.reserve(_maxTextureCount);
}

void Raytracer::initShaderBindingTable()
{
	_shaderBindingTable = ShaderBindingTable(_raytraceShader.get(), _raytraceShader.missShaderCount(), _raytraceShader.hitShaderCount(),
		_vulkan._device, _vulkan._physicalDevice, _vulkan._generalPurposeAllocator);
}

void Raytracer::loadPlaceholderMeshAndTexture()
{
	Vertex vertex0, vertex1, vertex2;
	vertex0.position = { -1.0f, -1.0f, 0.0f, 1.0f};
	vertex1.position = { -0.0f, -1.0f, 0.0f, 1.0f };
	vertex2.position = { 0.0f, 1.0f, 0.0f, 1.0f };
	std::vector<Vertex> vertices = {vertex0, vertex1, vertex2};
	std::vector<uint32_t> indices = { 0, 1, 2 };
	std::vector<unsigned char> pixels(128 * 128 * 4, 0);
	uint32_t index = loadTexture(pixels, 128, 128, 4, "placeHolderTexture");

	_dummyMesh = loadMesh(vertices, indices, "placeHolderMesh");
	_dummyMesh.material.albedoIndex = index;
	_dummyMesh.material.normalIndex = index;
	_dummyMesh.material.metallicIndex = index;
	_dummyMesh.material.roughnessIndex = index;

	vkDeviceWaitIdle(_vulkan._device);
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
	globalbufferInfo.range = VK_WHOLE_SIZE;

	VkDescriptorBufferInfo camerabufferInfo;
	camerabufferInfo.buffer = currentCameraBuffer->get();
	camerabufferInfo.offset = 0;
	camerabufferInfo.range = VK_WHOLE_SIZE;

	VkDescriptorImageInfo linearSamplerInfo = {};
	linearSamplerInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	linearSamplerInfo.sampler = _linearSampler;

	VkDescriptorImageInfo nearestSamplerInfo = {};
	nearestSamplerInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	nearestSamplerInfo.sampler = _nearestNeighborSampler;

	VkDescriptorBufferInfo drawDataInstanceInfo;
	drawDataInstanceInfo.buffer = currentDrawDataInstanceBuffer->get();
	drawDataInstanceInfo.offset = 0;
	drawDataInstanceInfo.range = VK_WHOLE_SIZE;

	VkDescriptorSet sets[] = { _globalDescriptorSets[_currentFrameIndex], _samplerDescriptorSets[_currentFrameIndex], _drawInstanceDescriptorSets[_currentFrameIndex]};

	VGM::DescriptorSetUpdater::begin(sets)
		.bindBuffer(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, camerabufferInfo, 1, 0, 0)
		.bindBuffer(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, globalbufferInfo, 1, 0, 1)
		.bindImage(VK_DESCRIPTOR_TYPE_SAMPLER, &linearSamplerInfo, nullptr, 1, 1, 0)
		.bindImage(VK_DESCRIPTOR_TYPE_SAMPLER, &nearestSamplerInfo, nullptr, 1, 1, 1)
		.bindBuffer(VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, drawDataInstanceInfo, 1, 2, 0)
		.updateDescriptorSet(_vulkan._device);
}

void Raytracer::updateDefferedDescriptorSets()
{
	VGM::Buffer* currentSunLightBuffer = &_sunLightBuffers[_currentFrameIndex];
	VGM::Buffer* currentPointLightBuffer = &_pointLightBuffers[_currentFrameIndex];
	VGM::Buffer* currentSpotLightBuffer = &_spotLightBuffers[_currentFrameIndex];

	VkDescriptorBufferInfo sunInfo = {};
	sunInfo.buffer = currentSunLightBuffer->get();
	sunInfo.offset = 0;
	sunInfo.range = VK_WHOLE_SIZE;

	VkDescriptorBufferInfo pointInfo = {};
	pointInfo.buffer = currentPointLightBuffer->get();
	pointInfo.offset = 0;
	pointInfo.range = VK_WHOLE_SIZE;

	VkDescriptorBufferInfo spotInfo = {};
	spotInfo.buffer = currentSpotLightBuffer->get();
	spotInfo.offset = 0;
	spotInfo.range = VK_WHOLE_SIZE;

	VkDescriptorSet sets[] = {_lightDescripotrSets[_currentFrameIndex] };

	VGM::DescriptorSetUpdater::begin(sets)
		.bindBuffer(VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, sunInfo, 1, 0, 0)
		.bindBuffer(VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, pointInfo, 1, 0, 1)
		.bindBuffer(VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, spotInfo, 1, 0, 2)
		.updateDescriptorSet(_vulkan._device);	
}

void Raytracer::updateRaytraceDescripotrSets()
{
	GBuffer* currentGBuffer = &_gBufferChain[_currentFrameIndex];

	VkDescriptorImageInfo colorInfo;
	colorInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	colorInfo.imageView = currentGBuffer->colorView;
	colorInfo.sampler = _linearSampler;

	VkDescriptorImageInfo normalInfo;
	normalInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	normalInfo.imageView = currentGBuffer->normalView;
	normalInfo.sampler = _linearSampler;

	VkDescriptorImageInfo depthInfo;
	depthInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	depthInfo.imageView = currentGBuffer->depthView;
	depthInfo.sampler = _linearSampler;

	VkDescriptorImageInfo roughnessMetalnessInfo;
	roughnessMetalnessInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	roughnessMetalnessInfo.imageView = currentGBuffer->roughnessMetalnessView;
	roughnessMetalnessInfo.sampler = _linearSampler;

	VkDescriptorImageInfo positionInfo;
	positionInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	positionInfo.imageView = currentGBuffer->positionView;
	positionInfo.sampler = _linearSampler;

	VkDescriptorImageInfo emissionInfo;
	emissionInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	emissionInfo.imageView = currentGBuffer->emissionView;
	emissionInfo.sampler = _linearSampler;

	VkDescriptorImageInfo outColorInfo;
	outColorInfo.imageLayout = VK_IMAGE_LAYOUT_GENERAL;
	outColorInfo.imageView = _historyBuffer.radianceViews[_historyBuffer.currentIndex];
	outColorInfo.sampler = VK_NULL_HANDLE;

	VkWriteDescriptorSetAccelerationStructureKHR accelWrite;
	accelWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET_ACCELERATION_STRUCTURE_KHR;
	accelWrite.pNext = nullptr;
	accelWrite.pAccelerationStructures = _accelerationStructure.topLevelAccelStructures[_currentFrameIndex].get();
	accelWrite.accelerationStructureCount = 1;
	
	VkDescriptorBufferInfo addressInfo = {};
	addressInfo.buffer = _meshBuffer.addressBuffer.get();
	addressInfo.offset = 0;
	addressInfo.range = sizeof(MeshBufferAddressData);

	VkDescriptorBufferInfo sampleSequenceInfo = {};
	sampleSequenceInfo.buffer = _sampleSequenceBuffer.get();
	sampleSequenceInfo.offset = 0;
	sampleSequenceInfo.range = VK_WHOLE_SIZE;

	VkDescriptorSet sets[] = { _raytracer1DescriptorSets[_currentFrameIndex], _raytracer2DescriptorSets[_currentFrameIndex] };
	VGM::DescriptorSetUpdater::begin(sets)
		.bindImage(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, &colorInfo, nullptr, 1, 0, 0)
		.bindImage(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, &normalInfo, nullptr, 1, 0, 1)
		.bindImage(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, &depthInfo, nullptr, 1, 0, 2)
		.bindImage(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, &roughnessMetalnessInfo, nullptr, 1, 0, 3)
		.bindImage(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, &positionInfo, nullptr, 1, 0, 4)
		.bindImage(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, &emissionInfo, nullptr, 1, 0, 5)
		.bindAccelerationStructure(VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR, &accelWrite, 1, 1, 0)
		.bindImage(VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, &outColorInfo, nullptr, 1, 1, 1)
		.bindBuffer(VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, addressInfo, 1, 1, 2)
		.bindBuffer(VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, sampleSequenceInfo, 1, 1, 4)
		.updateDescriptorSet(_vulkan._device);
}


void Raytracer::updatePostProcessDescriptorSets()
{
	
	std::vector<VkDescriptorImageInfo> velocityHistoryInfos;
	velocityHistoryInfos.resize(_historyLength);
	for(unsigned int i = 0; i<_historyLength; i++)
	{
		VkDescriptorImageInfo velocityHistoryInfo;
		velocityHistoryInfo.imageLayout = VK_IMAGE_LAYOUT_GENERAL;
		velocityHistoryInfo.imageView = _historyBuffer.velocityViews[i];
		velocityHistoryInfo.sampler = VK_NULL_HANDLE;
		velocityHistoryInfos[i] = velocityHistoryInfo;
	}

	std::vector<VkDescriptorImageInfo> radianceHistoryInfos;
	radianceHistoryInfos.resize(_historyLength);
	for (unsigned int i = 0; i < _historyLength; i++)
	{
		VkDescriptorImageInfo radianceHistoryInfo;
		radianceHistoryInfo.imageLayout = VK_IMAGE_LAYOUT_GENERAL;
		radianceHistoryInfo.imageView = _historyBuffer.radianceViews[i];
		radianceHistoryInfo.sampler = VK_NULL_HANDLE;
		radianceHistoryInfos[i] = radianceHistoryInfo;
	}

	VkDescriptorBufferInfo globalbufferInfo;
	globalbufferInfo.buffer = _globalRenderDataBuffers[_currentFrameIndex].get();
	globalbufferInfo.offset = 0;
	globalbufferInfo.range = VK_WHOLE_SIZE;

	VkDescriptorSet sets[] = { _postProcessDescriptorSets[_currentFrameIndex]};
	VGM::DescriptorSetUpdater::begin(sets)
		.bindBuffer(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, globalbufferInfo, 1, 0, 0)
		.bindImage(VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, radianceHistoryInfos.data(), nullptr, _historyLength, 0, 1)
		.bindImage(VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, velocityHistoryInfos.data(), nullptr, _historyLength, 0, 2)
		.updateDescriptorSet(_vulkan._device);
}

void Raytracer::executeDefferedPass()
{
	GBuffer* currentGBuffer = &_gBufferChain[_currentFrameIndex];
	
	VkSemaphore presentSemaphore = _frameSynchroStructs[_currentFrameIndex]._presentSemaphore;

	VGM::Buffer* currentDrawDataInstanceBuffer = &_drawDataIsntanceBuffers[_currentFrameIndex];
	VGM::Buffer* currentDrawIndirectBuffer = &_drawIndirectCommandBuffer[_currentFrameIndex];
	VGM::DescriptorSetAllocator* currentDefferedDescriptorSetAllocator = &_defferedDescriptorSetAllocator;

	VGM::CommandBuffer* defferedCmd = &_defferedRenderCommandBuffers[_currentFrameIndex];

	VkRect2D renderArea = { 0, 0, nativeWidth, nativeHeight };
	
	VkFence fences[] = {_frameSynchroStructs[_currentFrameIndex]._frameFence};


	VK_CHECK(defferedCmd->begin(fences, std::size(fences), _vulkan._device));


	_drawIndirectCommandBuffer[_currentFrameIndex].uploadData(_drawCommandTransferCache.data(),
		_drawCommandTransferCache.size() * sizeof(VkDrawIndexedIndirectCommand),
		_vulkan._generalPurposeAllocator);

	_drawDataIsntanceBuffers[_currentFrameIndex].uploadData(_drawDataTransferCache.data(),
		_drawDataTransferCache.size() * sizeof(DrawData),
		_vulkan._generalPurposeAllocator);

	_sunLightBuffers[_currentFrameIndex].uploadData(_sunLightTransferCache.data(),
		_sunLightTransferCache.size() * sizeof(SunLight), _vulkan._generalPurposeAllocator);

	_pointLightBuffers[_currentFrameIndex].uploadData(_pointLightTransferCache.data(),
		_pointLightTransferCache.size() * sizeof(PointLight), _vulkan._generalPurposeAllocator);

	_spotLightBuffers[_currentFrameIndex].uploadData(_spotLightTransferCache.data(),
		_spotLightTransferCache.size() * sizeof(SpotLight), _vulkan._generalPurposeAllocator);

	_globalRenderData.sunLightCount = static_cast<uint32_t>(_sunLightTransferCache.size());
	_globalRenderData.pointLightCount = static_cast<uint32_t>(_pointLightTransferCache.size());
	_globalRenderData.spotLightCount = static_cast<uint32_t>(_spotLightTransferCache.size());
	_globalRenderData.maxRecoursionDepth = _maxRecoursionDepth;
	_globalRenderData.maxDiffuseSampleCount = _diffuseSampleCount;
	_globalRenderData.maxSpecularSampleCount = _specularSampleCount;
	_globalRenderData.maxShadowRaySampleCount = _shadowSampleCount;
	_globalRenderData.sampleSequenceLength = _sampleSequenceLength;
	_globalRenderData.frameNumber = _frameNumber;
	_globalRenderData.historyLength = _historyLength;
	_globalRenderData.historyIndex = _historyBuffer.currentIndex;
	_globalRenderData.nativeResolutionWidth = nativeWidth;
	_globalRenderData.nativeResolutionHeight = nativeHeight;

	_cameraBuffers[_currentFrameIndex].uploadData(&_cameraData, sizeof(CameraData), _vulkan._generalPurposeAllocator);
	_globalRenderDataBuffers[_currentFrameIndex].uploadData(&_globalRenderData, sizeof(GlobalRenderData), _vulkan._generalPurposeAllocator);

	/*updateGBufferDescriptorSets();
	updateDefferedDescriptorSets();*/

	updateGBufferFramebufferBindings();

	/*_drawIndirectCommandBuffer[_currentFrameIndex].cmdMemoryBarrier(defferedCmd->get(), VK_ACCESS_HOST_WRITE_BIT, VK_ACCESS_SHADER_READ_BIT,
		VK_PIPELINE_STAGE_HOST_BIT, VK_PIPELINE_STAGE_VERTEX_SHADER_BIT, 0);
	_drawDataIsntanceBuffers[_currentFrameIndex].cmdMemoryBarrier(defferedCmd->get(), VK_ACCESS_HOST_WRITE_BIT, VK_ACCESS_SHADER_READ_BIT,
		VK_PIPELINE_STAGE_HOST_BIT, VK_PIPELINE_STAGE_VERTEX_SHADER_BIT | VK_PIPELINE_STAGE_RAY_TRACING_SHADER_BIT_KHR, 0);
	

	_sunLightBuffers[_currentFrameIndex].cmdMemoryBarrier(defferedCmd->get(),
		VK_ACCESS_HOST_WRITE_BIT, VK_ACCESS_SHADER_READ_BIT, VK_PIPELINE_STAGE_HOST_BIT, 
		VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT | VK_PIPELINE_STAGE_RAY_TRACING_SHADER_BIT_KHR, 0);
	_pointLightBuffers[_currentFrameIndex].cmdMemoryBarrier(defferedCmd->get(),
		VK_ACCESS_HOST_WRITE_BIT, VK_ACCESS_SHADER_READ_BIT, VK_PIPELINE_STAGE_HOST_BIT, 
		VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT | VK_PIPELINE_STAGE_RAY_TRACING_SHADER_BIT_KHR, 0);
	_pointLightBuffers[_currentFrameIndex].cmdMemoryBarrier(defferedCmd->get(),
		VK_ACCESS_HOST_WRITE_BIT, VK_ACCESS_SHADER_READ_BIT, VK_PIPELINE_STAGE_HOST_BIT, 
		VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT | VK_PIPELINE_STAGE_RAY_TRACING_SHADER_BIT_KHR, 0);

	_cameraBuffers[_currentFrameIndex].cmdMemoryBarrier(defferedCmd->get(), VK_ACCESS_HOST_WRITE_BIT, VK_ACCESS_SHADER_READ_BIT,
		VK_PIPELINE_STAGE_HOST_BIT, VK_PIPELINE_STAGE_VERTEX_SHADER_BIT | VK_PIPELINE_STAGE_RAY_TRACING_SHADER_BIT_KHR, 0);
	_globalRenderDataBuffers[_currentFrameIndex].cmdMemoryBarrier(defferedCmd->get(), VK_ACCESS_HOST_WRITE_BIT, VK_ACCESS_SHADER_READ_BIT,
		VK_PIPELINE_STAGE_HOST_BIT, VK_PIPELINE_STAGE_VERTEX_SHADER_BIT | VK_PIPELINE_STAGE_RAY_TRACING_SHADER_BIT_KHR | VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, 0);*/

	VkBufferMemoryBarrier2 drawIndirectBarrier = _drawIndirectCommandBuffer[_currentFrameIndex].createBufferMemoryBarrier2(VK_WHOLE_SIZE, 0,
		VK_ACCESS_2_HOST_WRITE_BIT, VK_ACCESS_2_SHADER_READ_BIT, VK_PIPELINE_STAGE_2_HOST_BIT, VK_PIPELINE_STAGE_2_VERTEX_SHADER_BIT,
		_vulkan._graphicsQueueFamilyIndex, _vulkan._graphicsQueueFamilyIndex);
	VkBufferMemoryBarrier2 drawInstanceBarrier = _drawDataIsntanceBuffers[_currentFrameIndex].createBufferMemoryBarrier2(VK_WHOLE_SIZE, 0,
		VK_ACCESS_2_HOST_WRITE_BIT, VK_ACCESS_2_SHADER_READ_BIT, VK_PIPELINE_STAGE_2_HOST_BIT, VK_PIPELINE_STAGE_2_VERTEX_SHADER_BIT,
		_vulkan._graphicsQueueFamilyIndex, _vulkan._graphicsQueueFamilyIndex);
	VkBufferMemoryBarrier2 sunBarrier = _sunLightBuffers[_currentFrameIndex].createBufferMemoryBarrier2(VK_WHOLE_SIZE, 0,
		VK_ACCESS_2_HOST_WRITE_BIT, VK_ACCESS_2_SHADER_READ_BIT, VK_PIPELINE_STAGE_2_HOST_BIT, VK_PIPELINE_STAGE_2_VERTEX_SHADER_BIT | VK_PIPELINE_STAGE_2_RAY_TRACING_SHADER_BIT_KHR,
		_vulkan._graphicsQueueFamilyIndex, _vulkan._graphicsQueueFamilyIndex);
	VkBufferMemoryBarrier2 pointBarrier = _pointLightBuffers[_currentFrameIndex].createBufferMemoryBarrier2(VK_WHOLE_SIZE, 0,
		VK_ACCESS_2_HOST_WRITE_BIT, VK_ACCESS_2_SHADER_READ_BIT, VK_PIPELINE_STAGE_2_HOST_BIT, VK_PIPELINE_STAGE_2_VERTEX_SHADER_BIT | VK_PIPELINE_STAGE_2_RAY_TRACING_SHADER_BIT_KHR,
		_vulkan._graphicsQueueFamilyIndex, _vulkan._graphicsQueueFamilyIndex);
	VkBufferMemoryBarrier2 spotBarrier = _spotLightBuffers[_currentFrameIndex].createBufferMemoryBarrier2(VK_WHOLE_SIZE, 0,
		VK_ACCESS_2_HOST_WRITE_BIT, VK_ACCESS_2_SHADER_READ_BIT, VK_PIPELINE_STAGE_2_HOST_BIT, VK_PIPELINE_STAGE_2_VERTEX_SHADER_BIT | VK_PIPELINE_STAGE_2_RAY_TRACING_SHADER_BIT_KHR,
		_vulkan._graphicsQueueFamilyIndex, _vulkan._graphicsQueueFamilyIndex);
	VkBufferMemoryBarrier2 cameraBarrier = _cameraBuffers[_currentFrameIndex].createBufferMemoryBarrier2(VK_WHOLE_SIZE, 0,
		VK_ACCESS_2_HOST_WRITE_BIT, VK_ACCESS_2_SHADER_READ_BIT, VK_PIPELINE_STAGE_2_HOST_BIT, VK_PIPELINE_STAGE_2_VERTEX_SHADER_BIT | VK_PIPELINE_STAGE_2_RAY_TRACING_SHADER_BIT_KHR,
		_vulkan._graphicsQueueFamilyIndex, _vulkan._graphicsQueueFamilyIndex);
	VkBufferMemoryBarrier2 globalDataBarrier = _globalRenderDataBuffers[_currentFrameIndex].createBufferMemoryBarrier2(VK_WHOLE_SIZE, 0,
		VK_ACCESS_2_HOST_WRITE_BIT, VK_ACCESS_2_SHADER_READ_BIT, VK_PIPELINE_STAGE_2_HOST_BIT, VK_PIPELINE_STAGE_2_VERTEX_SHADER_BIT | VK_PIPELINE_STAGE_2_RAY_TRACING_SHADER_BIT_KHR,
		_vulkan._graphicsQueueFamilyIndex, _vulkan._graphicsQueueFamilyIndex);

	VkBufferMemoryBarrier2 bufferBarriers[] = { drawIndirectBarrier, drawInstanceBarrier, sunBarrier, pointBarrier, spotBarrier, cameraBarrier, globalDataBarrier };

	VkImageSubresourceRange subresource;
	subresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	subresource.baseArrayLayer = 0;
	subresource.baseMipLevel = 0;
	subresource.layerCount = 1;
	subresource.levelCount = 1;

	VkImageMemoryBarrier2 colorBarrier = currentGBuffer->colorBuffer.createImageMemoryBarrier2(subresource, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
		VK_ACCESS_2_NONE, VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT, VK_PIPELINE_STAGE_2_NONE, VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT,
		_vulkan._graphicsQueueFamilyIndex, _vulkan._graphicsQueueFamilyIndex);
	VkImageMemoryBarrier2 rougnessBarrier = currentGBuffer->roughnessMetalnessBuffer.createImageMemoryBarrier2(subresource, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
		VK_ACCESS_2_NONE, VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT, VK_PIPELINE_STAGE_2_NONE, VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT,
		_vulkan._graphicsQueueFamilyIndex, _vulkan._graphicsQueueFamilyIndex);
	VkImageMemoryBarrier2 normalBarrier = currentGBuffer->normalBuffer.createImageMemoryBarrier2(subresource, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
		VK_ACCESS_2_NONE, VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT, VK_PIPELINE_STAGE_2_NONE, VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT,
		_vulkan._graphicsQueueFamilyIndex, _vulkan._graphicsQueueFamilyIndex);
	VkImageMemoryBarrier2 positionBarrier = currentGBuffer->positionBuffer.createImageMemoryBarrier2(subresource, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
		VK_ACCESS_2_NONE, VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT, VK_PIPELINE_STAGE_2_NONE, VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT,
		_vulkan._graphicsQueueFamilyIndex, _vulkan._graphicsQueueFamilyIndex);
	VkImageMemoryBarrier2 emissionBarrier = currentGBuffer->emissionBuffer.createImageMemoryBarrier2(subresource, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
		VK_ACCESS_2_NONE, VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT, VK_PIPELINE_STAGE_2_NONE, VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT,
	_vulkan._graphicsQueueFamilyIndex, _vulkan._graphicsQueueFamilyIndex);
	
	subresource.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
	VkImageMemoryBarrier2 depthBarrier = currentGBuffer->depthBuffer.createImageMemoryBarrier2(subresource, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL,
		VK_ACCESS_2_NONE, VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT, VK_PIPELINE_STAGE_2_NONE,
		VK_PIPELINE_STAGE_2_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_2_LATE_FRAGMENT_TESTS_BIT,
		_vulkan._graphicsQueueFamilyIndex, _vulkan._graphicsQueueFamilyIndex);

	VkImageMemoryBarrier2 imageBarriers[] = { colorBarrier, rougnessBarrier, normalBarrier, positionBarrier, emissionBarrier, depthBarrier };

	/*currentGBuffer->colorBuffer.cmdTransitionLayout(defferedCmd->get(), subresource, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
		VK_ACCESS_NONE, VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT);

	currentGBuffer->roughnessMetalnessBuffer.cmdTransitionLayout(defferedCmd->get(), subresource, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
		VK_ACCESS_NONE, VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT);

	currentGBuffer->normalBuffer.cmdTransitionLayout(defferedCmd->get(), subresource, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
		VK_ACCESS_NONE, VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT);

	currentGBuffer->positionBuffer.cmdTransitionLayout(defferedCmd->get(), subresource, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
		VK_ACCESS_NONE, VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT);

	currentGBuffer->emissionBuffer.cmdTransitionLayout(defferedCmd->get(), subresource, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
		VK_ACCESS_NONE, VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT);*/
	
	VkDependencyInfo dependency = {};
	dependency.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO;
	dependency.pNext = nullptr;
	dependency.bufferMemoryBarrierCount = std::size(bufferBarriers);
	dependency.pBufferMemoryBarriers = bufferBarriers;
	dependency.memoryBarrierCount = 0;
	dependency.imageMemoryBarrierCount = std::size(imageBarriers);
	dependency.pImageMemoryBarriers = imageBarriers;

	vkCmdPipelineBarrier2(defferedCmd->get(), &dependency);

	subresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	_historyBuffer.velocityHistoryBuffer[_historyBuffer.currentIndex].cmdTransitionLayout(defferedCmd->get(), subresource, VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
		VK_ACCESS_NONE, VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT);
	

	/*subresource.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
	currentGBuffer->depthBuffer.cmdTransitionLayout(defferedCmd->get(), subresource, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL,
		VK_ACCESS_NONE, VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT,
		VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT);*/

	

	VkDescriptorSet offscrenSets[] = { _globalDescriptorSets[_currentFrameIndex],
		_samplerDescriptorSets[_currentFrameIndex],
		_drawInstanceDescriptorSets[_currentFrameIndex],
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
		VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT, VK_ACCESS_SHADER_READ_BIT,
		VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
		VK_PIPELINE_STAGE_RAY_TRACING_SHADER_BIT_KHR);

	currentGBuffer->roughnessMetalnessBuffer.cmdTransitionLayout(defferedCmd->get(), subresource,
		VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
		VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT, VK_ACCESS_SHADER_READ_BIT,
		VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
		VK_PIPELINE_STAGE_RAY_TRACING_SHADER_BIT_KHR);

	currentGBuffer->normalBuffer.cmdTransitionLayout(defferedCmd->get(), subresource,
		VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
		VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT, VK_ACCESS_SHADER_READ_BIT,
		VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
		VK_PIPELINE_STAGE_RAY_TRACING_SHADER_BIT_KHR);

	currentGBuffer->positionBuffer.cmdTransitionLayout(defferedCmd->get(), subresource,
		VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
		VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT, VK_ACCESS_SHADER_READ_BIT,
		VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
		VK_PIPELINE_STAGE_RAY_TRACING_SHADER_BIT_KHR);

	currentGBuffer->emissionBuffer.cmdTransitionLayout(defferedCmd->get(), subresource,
		VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
		VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT, VK_ACCESS_SHADER_READ_BIT,
		VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
		VK_PIPELINE_STAGE_RAY_TRACING_SHADER_BIT_KHR);

	_historyBuffer.velocityHistoryBuffer[_historyBuffer.currentIndex].cmdTransitionLayout(defferedCmd->get(), subresource, 
		VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_GENERAL,
		VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT, VK_ACCESS_SHADER_READ_BIT, 
		VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT);

	subresource.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
	currentGBuffer->depthBuffer.cmdTransitionLayout(defferedCmd->get(), subresource, VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
		VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT, VK_ACCESS_SHADER_READ_BIT,
		VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT, VK_PIPELINE_STAGE_RAY_TRACING_SHADER_BIT_KHR);

	defferedCmd->end();

	VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
	VK_CHECK(defferedCmd->submit(&_frameSynchroStructs[_currentFrameIndex]._defferedRenderSemaphore, 1, &presentSemaphore, 1, waitStages, nullptr, _vulkan._graphicsQeueu));

	_sunLightTransferCache.clear();
	_pointLightTransferCache.clear();
	_spotLightTransferCache.clear();
	_drawCommandTransferCache.clear();
	_drawDataTransferCache.clear();
}

void Raytracer::executeRaytracePass()
{
		VGM::CommandBuffer* raytraceCmd = &_raytraceRenderCommandBuffers[_currentFrameIndex];
		
		VkSemaphore raytraceSemaphore = _frameSynchroStructs[_currentFrameIndex]._raytraceSemaphore;
		VkSemaphore defferedSemaphore = _frameSynchroStructs[_currentFrameIndex]._defferedRenderSemaphore;

		GBuffer* currentGBuffer = &_gBufferChain[_currentFrameIndex];
		RaytraceBuffer* currentRaytraceBuffer = &_raytraceBufferChain[_currentFrameIndex];

		TLAS* currentTopLevelAccelStructure = &_accelerationStructure.topLevelAccelStructures[_currentFrameIndex];
		VGM::Buffer* currentInstanceBuffer = &_accelerationStructure.instanceBuffers[_currentFrameIndex];

		VkDescriptorSet sets[] = {
			_globalDescriptorSets[_currentFrameIndex],
			_raytracer1DescriptorSets[_currentFrameIndex],
			_raytracer2DescriptorSets[_currentFrameIndex],
			_lightDescripotrSets[_currentFrameIndex],
			_samplerDescriptorSets[_currentFrameIndex],
			_drawInstanceDescriptorSets[_currentFrameIndex],
			_textureDescriptorSets[_currentFrameIndex]
		};

		VK_CHECK(raytraceCmd->begin(nullptr, 0, _vulkan._device));

		void* ptr = currentInstanceBuffer->map(_vulkan._generalPurposeAllocator);
		currentInstanceBuffer->memcopy(ptr, _accelerationStructure._instanceTransferCache.data(),
			sizeof(VkAccelerationStructureInstanceKHR) * _accelerationStructure._instanceTransferCache.size());
		currentInstanceBuffer->unmap(_vulkan._generalPurposeAllocator);

		currentInstanceBuffer->cmdMemoryBarrier(raytraceCmd->get(), VK_ACCESS_HOST_WRITE_BIT, VK_ACCESS_ACCELERATION_STRUCTURE_WRITE_BIT_KHR | VK_ACCESS_ACCELERATION_STRUCTURE_READ_BIT_KHR,
			VK_PIPELINE_STAGE_HOST_BIT, VK_PIPELINE_STAGE_ACCELERATION_STRUCTURE_BUILD_BIT_KHR, 0);

		VkDeviceAddress firstAddress = currentInstanceBuffer->getDeviceAddress(_vulkan._device);

		currentTopLevelAccelStructure->cmdUpdateTLAS(_accelerationStructure._instanceTransferCache.size(),
			firstAddress, _vulkan._physicalDevice, _vulkan._device, _vulkan._generalPurposeAllocator, raytraceCmd->get());


		updateRaytraceDescripotrSets();

		VkImageSubresourceRange subresource;
		subresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		subresource.baseArrayLayer = 0;
		subresource.baseMipLevel = 0;
		subresource.layerCount = 1;
		subresource.levelCount = 1;
		
		_historyBuffer.velocityHistoryBuffer[_historyBuffer.currentIndex].cmdTransitionLayout(raytraceCmd->get(), subresource,
			VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_LAYOUT_GENERAL, VK_ACCESS_SHADER_READ_BIT, VK_ACCESS_SHADER_WRITE_BIT,
			VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_RAY_TRACING_SHADER_BIT_KHR);

		_raytraceShader.cmdBind(raytraceCmd->get());
		vkCmdBindDescriptorSets(raytraceCmd->get(), VK_PIPELINE_BIND_POINT_RAY_TRACING_KHR, _raytracePipelineLayout, 0, std::size(sets), sets, 0, nullptr);

		VkStridedDeviceAddressRegionKHR callableRegion = {};
		VkStridedDeviceAddressRegionKHR raygenRegion = _shaderBindingTable.getRaygenRegion();
		VkStridedDeviceAddressRegionKHR hitRegion = _shaderBindingTable.getHitRegion();
		VkStridedDeviceAddressRegionKHR missRegion = _shaderBindingTable.getMissRegion();

		vkCmdTraceRaysKHR(raytraceCmd->get(),
			&raygenRegion, &missRegion,
			&hitRegion, &callableRegion,
			nativeWidth, nativeHeight, _maxRecoursionDepth);

		_historyBuffer.velocityHistoryBuffer[_historyBuffer.currentIndex].cmdTransitionLayout(raytraceCmd->get(), subresource,
			VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_LAYOUT_GENERAL, VK_ACCESS_SHADER_WRITE_BIT, VK_ACCESS_SHADER_READ_BIT,
			VK_PIPELINE_STAGE_RAY_TRACING_SHADER_BIT_KHR, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT);

		currentGBuffer->normalBuffer.cmdTransitionLayout(raytraceCmd->get(), subresource,
			VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_IMAGE_LAYOUT_GENERAL,
			VK_ACCESS_SHADER_READ_BIT, VK_ACCESS_SHADER_READ_BIT,
			VK_PIPELINE_STAGE_RAY_TRACING_SHADER_BIT_KHR,
			VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT);

		currentGBuffer->positionBuffer.cmdTransitionLayout(raytraceCmd->get(), subresource,
			VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_IMAGE_LAYOUT_GENERAL,
			VK_ACCESS_SHADER_READ_BIT, VK_ACCESS_SHADER_READ_BIT,
			VK_PIPELINE_STAGE_RAY_TRACING_SHADER_BIT_KHR,
			VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT);

		raytraceCmd->end();

		VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_RAY_TRACING_SHADER_BIT_KHR };
		VK_CHECK(raytraceCmd->submit(&_frameSynchroStructs[_currentFrameIndex]._raytraceSemaphore, 1, &_frameSynchroStructs[_currentFrameIndex]._defferedRenderSemaphore, 1, waitStages, nullptr, _vulkan._computeQueue));

		_accelerationStructure._instanceTransferCache.clear();
	
}

void Raytracer::executePostProcessPass()
{
	RaytraceBuffer* currentRaytraceBuffer = &_raytraceBufferChain[_currentFrameIndex];

	VGM::CommandBuffer* postProcessCmd = &_postProcessCommandBuffers[_currentFrameIndex];
	VkDescriptorSet currentpostProcessDescriptorSet = _postProcessDescriptorSets[_currentFrameIndex];

	VkFence currentFence = _frameSynchroStructs[_currentFrameIndex]._frameFence;
	VkSemaphore currentPostProcessSemaphore = _frameSynchroStructs[_currentFrameIndex]._postProcessSemaphore;

	postProcessCmd->begin(nullptr, 0, _vulkan._device);
	
	/*updatePostProcessDescriptorSets();*/

	VkDescriptorSet sets[] =
	{
		_postProcessDescriptorSets[_currentFrameIndex],
		_postProcessInputOutputDescriptorSets[_currentFrameIndex]
	};
	
	VkImageSubresourceRange subresource;
	subresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	subresource.baseArrayLayer = 0;
	subresource.baseMipLevel = 0;
	subresource.layerCount = 1;
	subresource.levelCount = 1;

	VkImageMemoryBarrier presentBarrier = {};
	presentBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
	presentBarrier.pNext = nullptr;
	presentBarrier.image = _vulkan._swapchainImages[_currentSwapchainIndex];
	presentBarrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	presentBarrier.newLayout = VK_IMAGE_LAYOUT_GENERAL;
	presentBarrier.subresourceRange = subresource;
	presentBarrier.srcAccessMask = VK_ACCESS_NONE;
	presentBarrier.dstAccessMask = VK_ACCESS_SHADER_WRITE_BIT;
	
	vkCmdPipelineBarrier(postProcessCmd->get(), VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, 0,
		0, nullptr,
		0, nullptr,
		1, &presentBarrier);

	_leapFrogBuffer.textures[0].cmdTransitionLayout(postProcessCmd->get(), subresource, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_GENERAL,
		VK_ACCESS_NONE, VK_ACCESS_SHADER_READ_BIT, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT);
	_leapFrogBuffer.textures[1].cmdTransitionLayout(postProcessCmd->get(), subresource, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_GENERAL,
		VK_ACCESS_NONE, VK_ACCESS_SHADER_WRITE_BIT, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT);

	unsigned int i = 0;

	for (auto p : _postProcessingChain)
	{
		VkDescriptorImageInfo inputInfo = {};
		inputInfo.imageLayout = VK_IMAGE_LAYOUT_GENERAL;
		inputInfo.imageView = _leapFrogBuffer.views[i % 2];
		
		VkDescriptorImageInfo outputInfo = {};
		outputInfo.imageLayout = VK_IMAGE_LAYOUT_GENERAL;
		outputInfo.imageView = _leapFrogBuffer.views[(i + 1) % 2];

		VGM::DescriptorSetUpdater::begin(&_postProcessInputOutputDescriptorSets[i + (_maxPostProcessChainLength) * _currentFrameIndex])
			.bindImage(VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, &inputInfo, nullptr, 1, 0, 0)
			.bindImage(VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, &outputInfo, nullptr, 1, 0, 1)
			.updateDescriptorSet(_vulkan._device);

		sets[1] = _postProcessInputOutputDescriptorSets[i + (_maxPostProcessChainLength) * _currentFrameIndex];

		p.cmdBind(postProcessCmd->get());
		vkCmdBindDescriptorSets(postProcessCmd->get(), VK_PIPELINE_BIND_POINT_COMPUTE, _postProcessPipelineLayout, 0, std::size(sets), sets, 0, nullptr);
		vkCmdDispatch(postProcessCmd->get(), nativeWidth / 16, nativeHeight / 16, 1);

		_leapFrogBuffer.textures[i % 2].cmdTransitionLayout(postProcessCmd->get(), subresource, VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_LAYOUT_GENERAL,
			VK_ACCESS_SHADER_READ_BIT, VK_ACCESS_SHADER_WRITE_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT);
		_leapFrogBuffer.textures[(i + 1) % 2].cmdTransitionLayout(postProcessCmd->get(), subresource, VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_LAYOUT_GENERAL,
			VK_ACCESS_SHADER_WRITE_BIT, VK_ACCESS_SHADER_READ_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT);

		i++;
	}
	
	VkDescriptorImageInfo inputInfo = {};
	inputInfo.imageLayout = VK_IMAGE_LAYOUT_GENERAL;
	inputInfo.imageView = _leapFrogBuffer.views[i % 2];

	VkDescriptorImageInfo outputInfo = {};
	outputInfo.imageLayout = VK_IMAGE_LAYOUT_GENERAL;
	outputInfo.imageView = _vulkan._swapchainImageViews[_currentSwapchainIndex];

	VGM::DescriptorSetUpdater::begin(&_postProcessInputOutputDescriptorSets[_postProcessInputOutputDescriptorSets.size() - 1 - _currentFrameIndex])
		.bindImage(VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, &inputInfo, nullptr, 1, 0, 0)
		.bindImage(VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, &outputInfo, nullptr, 1, 0, 1)
		.updateDescriptorSet(_vulkan._device);

	sets[1] = _postProcessInputOutputDescriptorSets[_postProcessInputOutputDescriptorSets.size() - 1 - _currentFrameIndex];

	_colorMapingShader.cmdBind(postProcessCmd->get());
	vkCmdBindDescriptorSets(postProcessCmd->get(), VK_PIPELINE_BIND_POINT_COMPUTE, _postProcessPipelineLayout, 0, std::size(sets), sets, 0, nullptr);
	vkCmdDispatch(postProcessCmd->get(), (nativeWidth * 2) / 16, (nativeHeight * 2) / 16, 1);

	presentBarrier.oldLayout = VK_IMAGE_LAYOUT_GENERAL;
	presentBarrier.newLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
	presentBarrier.subresourceRange = subresource;
	presentBarrier.srcAccessMask = VK_ACCESS_SHADER_WRITE_BIT;
	presentBarrier.dstAccessMask = VK_ACCESS_NONE;

	vkCmdPipelineBarrier(postProcessCmd->get(), VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, 0,
		0, nullptr,
		0, nullptr,
		1, &presentBarrier);

	postProcessCmd->end();
	
	

	VkPipelineStageFlags waitStageFlags = VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT;
	VK_CHECK(postProcessCmd->submit(&currentPostProcessSemaphore, 1, &_frameSynchroStructs[_currentFrameIndex]._raytraceSemaphore, 1, &waitStageFlags, currentFence, _vulkan._computeQueue));
}

void Raytracer::initSamplers()
{
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
	createInfo.maxLod = 100.0f;
	createInfo.minLod = 0;
	createInfo.mipLodBias = 0;
	createInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
	createInfo.unnormalizedCoordinates = VK_FALSE;

	vkCreateSampler(_vulkan._device, &createInfo, nullptr, &_linearSampler);

	createInfo.minFilter = VK_FILTER_NEAREST;
	createInfo.magFilter = VK_FILTER_NEAREST;
	createInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_NEAREST;

	vkCreateSampler(_vulkan._device, &createInfo, nullptr, &_nearestNeighborSampler);

	createInfo.minLod = std::floor((float)_maxMipMapLevels/2.0);
	vkCreateSampler(_vulkan._device, &createInfo, nullptr, &_lowFidelitySampler);
}

void Raytracer::updateDescriptorSets()
{
	_currentFrameIndex = 0;
	updateGBufferDescriptorSets();
	updateDefferedDescriptorSets();
	updatePostProcessDescriptorSets();
	_currentFrameIndex = 1;
	updateGBufferDescriptorSets();
	updateDefferedDescriptorSets();
	updatePostProcessDescriptorSets();
	_currentFrameIndex = 0;
	//updateRaytraceDescripotrSets();
}

void Raytracer::genHaltonSequence(uint32_t a, uint32_t b, uint32_t length)
{
	std::vector<glm::vec2> samples;
	samples.resize(length);
	for(unsigned int i = 0; i<length; i++)
	{
		glm::vec2 sample = { genHaltonSample(a, i + 10), genHaltonSample(b, i + 10) };
		samples[i] = sample;
	}

	_sampleSequenceBuffer = VGM::Buffer(VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, VMA_ALLOCATION_CREATE_HOST_ACCESS_RANDOM_BIT, length * sizeof(glm::vec2),
		_vulkan._generalPurposeAllocator, _vulkan._device);

	void* ptr = _sampleSequenceBuffer.map(_vulkan._generalPurposeAllocator);
	_sampleSequenceBuffer.memcopy(ptr, samples.data(), samples.size() * sizeof(glm::vec2));
	_sampleSequenceBuffer.unmap(_vulkan._generalPurposeAllocator);
}

float Raytracer::genHaltonSample(uint32_t base, uint32_t index)
{
	float f = 1;
	float r = 0;

	while(index > 0)
	{
		f = f / static_cast<float>(base);
		r = r + f * (index % base);
		index = std::floor(static_cast<float>(index) / static_cast<float>(base));
	}

	return r;
}

void Raytracer::initHistoryBuffers()
{
	_historyBuffer.velocityHistoryBuffer.resize(_historyLength);
	_historyBuffer.velocityViews.resize(_historyLength);
	_historyBuffer.radianceHistoryBuffer.resize(_historyLength);
	_historyBuffer.radianceViews.resize(_historyLength);

	 for(unsigned int i = 0; i<_historyLength; i++)
	 {
		 _historyBuffer.velocityHistoryBuffer[i] = VGM::Texture(VK_FORMAT_R32G32B32A32_SFLOAT, VK_IMAGE_TYPE_2D, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_STORAGE_BIT,
			 { nativeWidth, nativeHeight, 1 }, 1, 1, _vulkan._gpuAllocator);
		 _historyBuffer.velocityHistoryBuffer[i].createImageView(0, 1, 0, 1, _vulkan._device, &_historyBuffer.velocityViews[i]);

		 _historyBuffer.radianceHistoryBuffer[i] = VGM::Texture(VK_FORMAT_R32G32B32A32_SFLOAT, VK_IMAGE_TYPE_2D, VK_IMAGE_USAGE_STORAGE_BIT,
			{ nativeWidth, nativeHeight, 1 }, 1, 1, _vulkan._gpuAllocator);
		 _historyBuffer.radianceHistoryBuffer[i].createImageView(0, 1, 0, 1, _vulkan._device, &_historyBuffer.radianceViews[i]);
	 }
}

void Raytracer::setEnvironmentTextureIndex(uint32_t index)
{
	_globalRenderData.environmentTextureIndex = index;
}

void Raytracer::updateGBufferFramebufferBindings()
{
	GBuffer* g = &_gBufferChain[_currentFrameIndex];

	VkClearColorValue clear = { 0.0f, 0.0f, 0.0f };

	g->framebuffer.resetBindings();

	g->framebuffer.bindColorTextureTarget(g->colorView, VK_ATTACHMENT_LOAD_OP_CLEAR, VK_ATTACHMENT_STORE_OP_STORE, clear);
	g->framebuffer.bindColorTextureTarget(g->normalView, VK_ATTACHMENT_LOAD_OP_CLEAR, VK_ATTACHMENT_STORE_OP_STORE, clear);
	g->framebuffer.bindColorTextureTarget(g->roughnessMetalnessView, VK_ATTACHMENT_LOAD_OP_CLEAR, VK_ATTACHMENT_STORE_OP_STORE, clear);
	g->framebuffer.bindColorTextureTarget(g->positionView, VK_ATTACHMENT_LOAD_OP_CLEAR, VK_ATTACHMENT_STORE_OP_STORE, clear);
	g->framebuffer.bindColorTextureTarget(g->emissionView, VK_ATTACHMENT_LOAD_OP_CLEAR, VK_ATTACHMENT_STORE_OP_STORE, clear);
	g->framebuffer.bindColorTextureTarget(_historyBuffer.velocityViews[_historyBuffer.currentIndex], 
		VK_ATTACHMENT_LOAD_OP_CLEAR, VK_ATTACHMENT_STORE_OP_STORE, clear);
	g->framebuffer.bindDepthTextureTarget(g->depthView, VK_ATTACHMENT_LOAD_OP_CLEAR, VK_ATTACHMENT_STORE_OP_STORE, 1.0f);
}

void Raytracer::initBufferState()
{
	_transferCommandBuffer = VGM::CommandBuffer(_transferCommandBufferAllocator, _vulkan._device);
	_transferCommandBuffer.begin(nullptr, 0, _vulkan._device);

	VkImageSubresourceRange subresource = {};
	subresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	subresource.baseArrayLayer = 0;
	subresource.baseMipLevel = 0;
	subresource.layerCount = 1;
	subresource.levelCount = 1;

	for(unsigned int i = 0; i<_historyBuffer.velocityHistoryBuffer.size(); i++)
	{
		_historyBuffer.velocityHistoryBuffer[i].cmdTransitionLayout(_transferCommandBuffer.get(), subresource,
			VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_GENERAL, VK_ACCESS_NONE, VK_ACCESS_SHADER_READ_BIT,
			VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT);

		_historyBuffer.radianceHistoryBuffer[i].cmdTransitionLayout(_transferCommandBuffer.get(), subresource,
			VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_GENERAL, VK_ACCESS_NONE, VK_ACCESS_SHADER_READ_BIT,
			VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT);
	}

	_transferCommandBuffer.end();

	VkPipelineStageFlags flags = 0;
	_transferCommandBuffer.submit(nullptr, 0, nullptr, 0, &flags, VK_NULL_HANDLE, _vulkan._transferQeueu);

	vkDeviceWaitIdle(_vulkan._device);

	_transferCommandBufferAllocator.reset(_vulkan._device);
}

void Raytracer::initPostProcessBuffers()
{
	_leapFrogBuffer.textures.resize(2);
	_leapFrogBuffer.views.resize(2);

	for(unsigned int i = 0; i<2; i++)
	{
		_leapFrogBuffer.textures[i] = VGM::Texture(VK_FORMAT_R32G32B32A32_SFLOAT, VK_IMAGE_TYPE_2D, VK_IMAGE_USAGE_STORAGE_BIT, 
			{ nativeWidth, nativeHeight, 1 }, 1, 1, _vulkan._textureAllocator);
		_leapFrogBuffer.textures[i].createImageView(0, 1, 0, 1, _vulkan._device, &_leapFrogBuffer.views[i]);
	}
}

void Raytracer::initgBuffers()
{
	_gBufferChain.resize(_concurrencyCount);
	for (auto& g : _gBufferChain)
	{
		g.colorBuffer = VGM::Texture(VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_TYPE_2D, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
			{ nativeWidth, nativeHeight, 1 }, 1, 1, _vulkan._gpuAllocator);
		g.normalBuffer = VGM::Texture(VK_FORMAT_R16G16B16A16_SFLOAT, VK_IMAGE_TYPE_2D, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_STORAGE_BIT,
			{ nativeWidth, nativeHeight, 1 }, 1, 1, _vulkan._gpuAllocator);
		g.roughnessMetalnessBuffer = VGM::Texture(VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_TYPE_2D, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
			{ nativeWidth, nativeHeight, 1 }, 1, 1, _vulkan._gpuAllocator);
		g.positionBuffer = VGM::Texture(VK_FORMAT_R32G32B32A32_SFLOAT, VK_IMAGE_TYPE_2D, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_STORAGE_BIT,
			{ nativeWidth, nativeHeight, 1 }, 1, 1, _vulkan._gpuAllocator);
		g.emissionBuffer = VGM::Texture(VK_FORMAT_R16G16B16A16_SFLOAT, VK_IMAGE_TYPE_2D, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_STORAGE_BIT,
			{ nativeWidth, nativeHeight, 1 }, 1, 1, _vulkan._gpuAllocator);
		g.depthBuffer = VGM::Texture(VK_FORMAT_D32_SFLOAT, VK_IMAGE_TYPE_2D, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
			{ nativeWidth, nativeHeight, 1 }, 1, 1, _vulkan._gpuAllocator);

		VkClearColorValue clear = { 1.0f, 1.0f, 1.0f };

		
		g.colorBuffer.createImageView(0, 1, 0, 1, _vulkan._device, &g.colorView);
		g.normalBuffer.createImageView(0, 1, 0, 1, _vulkan._device, &g.normalView);
		g.roughnessMetalnessBuffer.createImageView(0, 1, 0, 1, _vulkan._device, &g.roughnessMetalnessView);
		g.positionBuffer.createImageView(0, 1, 0, 1, _vulkan._device, &g.positionView);
		g.emissionBuffer.createImageView(0, 1, 0, 1, _vulkan._device, &g.emissionView);
		g.depthBuffer.createImageView(0, 1, 0, 1, _vulkan._device, &g.depthView);
	
	}
}

void Raytracer::initDefferedBuffers()
{
	_defferdBufferChain.resize(_concurrencyCount);
	for(auto& d : _defferdBufferChain)
	{
		d.colorBuffer = VGM::Texture(VK_FORMAT_R32G32B32A32_SFLOAT, VK_IMAGE_TYPE_2D, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
			{ nativeWidth, nativeHeight, 1 }, 1, 1, _vulkan._gpuAllocator);
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
		r.colorBuffer = VGM::Texture(VK_FORMAT_R32G32B32A32_SFLOAT, VK_IMAGE_TYPE_3D, VK_IMAGE_USAGE_STORAGE_BIT,
			{ nativeWidth, nativeHeight, _historyLength }, 1, 1, _vulkan._gpuAllocator);
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
	_postProcessDescriptorSetAllocator = VGM::DescriptorSetAllocator(poolSizes, 100, 0, 10, _vulkan._device);
}

void Raytracer::initDescriptorSetLayouts()
{
	VGM::DescriptorSetLayoutBuilder::begin()
		.addBinding(0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, nullptr, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT 
			| VK_SHADER_STAGE_RAYGEN_BIT_KHR | VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR, 1)
		.addBinding(1, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, nullptr, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT 
			| VK_SHADER_STAGE_RAYGEN_BIT_KHR | VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR | VK_SHADER_STAGE_MISS_BIT_KHR, 1)
		.createDescriptorSetLayout(_vulkan._device, &_globalLayout);

	VGM::DescriptorSetLayoutBuilder::begin()
		.addBinding(0, VK_DESCRIPTOR_TYPE_SAMPLER, nullptr, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT | VK_SHADER_STAGE_RAYGEN_BIT_KHR
			| VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR | VK_SHADER_STAGE_MISS_BIT_KHR, 1)
		.addBinding(1, VK_DESCRIPTOR_TYPE_SAMPLER, nullptr, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT | VK_SHADER_STAGE_RAYGEN_BIT_KHR
			| VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR, 1)
		.createDescriptorSetLayout(_vulkan._device, &_samplerLayout);

	VGM::DescriptorSetLayoutBuilder::begin()
		.addBinding(0, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, nullptr, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT 
			| VK_SHADER_STAGE_RAYGEN_BIT_KHR | VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR, 1)
		.createDescriptorSetLayout(_vulkan._device, &_drawInstanceLayout);

	VGM::DescriptorSetLayoutBuilder::begin()
		.addBinding(0, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, nullptr, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT 
			| VK_SHADER_STAGE_RAYGEN_BIT_KHR | VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR, 1)
		.addBinding(1, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, nullptr, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT 
			| VK_SHADER_STAGE_RAYGEN_BIT_KHR | VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR, 1)
		.addBinding(2, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, nullptr, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT 
			| VK_SHADER_STAGE_RAYGEN_BIT_KHR | VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR, 1)
		.createDescriptorSetLayout(_vulkan._device, &_lightLayout);

	VGM::DescriptorSetLayoutBuilder::begin()
		.addBinding(0, VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, nullptr, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT 
			| VK_SHADER_STAGE_RAYGEN_BIT_KHR | VK_SHADER_STAGE_MISS_BIT_KHR | VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR, _maxTextureCount)
		.createDescriptorSetLayout(_vulkan._device, &_textureLayout);

	VGM::DescriptorSetLayoutBuilder::begin()
		.addBinding(0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, nullptr, VK_SHADER_STAGE_RAYGEN_BIT_KHR, 1)
		.addBinding(1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, nullptr, VK_SHADER_STAGE_RAYGEN_BIT_KHR, 1)
		.addBinding(2, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, nullptr, VK_SHADER_STAGE_RAYGEN_BIT_KHR, 1)
		.addBinding(3, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, nullptr, VK_SHADER_STAGE_RAYGEN_BIT_KHR, 1)
		.addBinding(4, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, nullptr, VK_SHADER_STAGE_RAYGEN_BIT_KHR, 1)
		.addBinding(5, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, nullptr, VK_SHADER_STAGE_RAYGEN_BIT_KHR, 1)
		.createDescriptorSetLayout(_vulkan._device, &_raytracerLayout1);

	VGM::DescriptorSetLayoutBuilder::begin()
		.addBinding(0, VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR, nullptr, VK_SHADER_STAGE_RAYGEN_BIT_KHR | VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR, 1)
		.addBinding(1, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, nullptr, VK_SHADER_STAGE_RAYGEN_BIT_KHR, 1)
		.addBinding(2, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, nullptr, VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR, 1)
		.addBinding(4, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, nullptr, VK_SHADER_STAGE_RAYGEN_BIT_KHR | VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR, 1)
		.createDescriptorSetLayout(_vulkan._device, &_raytracerLayout2);

	VGM::DescriptorSetLayoutBuilder::begin()
		.addBinding(0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, nullptr, VK_SHADER_STAGE_COMPUTE_BIT, 1)
		.addBinding(1, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, nullptr, VK_SHADER_STAGE_COMPUTE_BIT, _historyLength)//radianceHistroy
		.addBinding(2, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, nullptr, VK_SHADER_STAGE_COMPUTE_BIT, _historyLength) //velocityHistory
		.createDescriptorSetLayout(_vulkan._device, &_postProcessLayout);

	VGM::DescriptorSetLayoutBuilder::begin()
		.addBinding(0, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, nullptr, VK_SHADER_STAGE_COMPUTE_BIT, 1) //priorPostProcessStage
		.addBinding(1, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, nullptr, VK_SHADER_STAGE_COMPUTE_BIT, 1) //thisPostProcessStage 
		.createDescriptorSetLayout(_vulkan._device, &_postProcessInputOutputLayout);
}

void Raytracer::initDescripotrSets()
{
	_globalDescriptorSets.resize(_concurrencyCount);
	_drawInstanceDescriptorSets.resize(_concurrencyCount);
	_samplerDescriptorSets.resize(_concurrencyCount);
	_defferedDescriptorSets.resize(_concurrencyCount);
	_lightDescripotrSets.resize(_concurrencyCount);
	_textureDescriptorSets.resize(_concurrencyCount);
	_raytracer1DescriptorSets.resize(_concurrencyCount);
	_raytracer2DescriptorSets.resize(_concurrencyCount);
	_postProcessDescriptorSets.resize(_concurrencyCount);
	

	for(unsigned int i = 0; i< _concurrencyCount; i++)
	{
		VGM::DescriptorSetBuilder::begin()

			.createDescriptorSet(_vulkan._device, &_globalDescriptorSets[i], &_globalLayout, _offsecreenDescriptorSetAllocator);
		VGM::DescriptorSetBuilder::begin()

			.createDescriptorSet(_vulkan._device, &_drawInstanceDescriptorSets[i], &_drawInstanceLayout, _offsecreenDescriptorSetAllocator);
		VGM::DescriptorSetBuilder::begin()
			.createDescriptorSet(_vulkan._device, &_samplerDescriptorSets[i],  &_samplerLayout, _offsecreenDescriptorSetAllocator);

		VGM::DescriptorSetBuilder::begin()
			.createDescriptorSet(_vulkan._device, &_lightDescripotrSets[i], &_lightLayout, _defferedDescriptorSetAllocator);

		VGM::DescriptorSetBuilder::begin()
			.createDescriptorSet(_vulkan._device, &_textureDescriptorSets[i], &_textureLayout, _defferedDescriptorSetAllocator);

		VGM::DescriptorSetBuilder::begin()
			.createDescriptorSet(_vulkan._device, &_raytracer1DescriptorSets[i], &_raytracerLayout1, _raytracerDescriptorSetAllocator);
		VGM::DescriptorSetBuilder::begin()
			.createDescriptorSet(_vulkan._device, &_raytracer2DescriptorSets[i], &_raytracerLayout2, _raytracerDescriptorSetAllocator);

		VGM::DescriptorSetBuilder::begin()
			.createDescriptorSet(_vulkan._device, &_postProcessDescriptorSets[i], &_postProcessLayout, _postProcessDescriptorSetAllocator);
	}

	_postProcessInputOutputDescriptorSets.resize((_maxPostProcessChainLength + 1) * _concurrencyCount);

	for(unsigned int i = 0; i < _postProcessInputOutputDescriptorSets.size(); i++)
	{
		VGM::DescriptorSetBuilder::begin()
			.createDescriptorSet(_vulkan._device, &_postProcessInputOutputDescriptorSets[i], &_postProcessInputOutputLayout, _postProcessDescriptorSetAllocator);
	}
}

void Raytracer::initGBufferShader()
{
	VkDescriptorSetLayout setLayouts[] = { _globalLayout, _samplerLayout, _drawInstanceLayout, _textureLayout};

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
	std::vector<VkFormat> renderingColorFormats = { VK_FORMAT_R8G8B8A8_UNORM, VK_FORMAT_R16G16B16A16_SFLOAT
		, VK_FORMAT_R8G8B8A8_UNORM, VK_FORMAT_R32G32B32A32_SFLOAT, VK_FORMAT_R16G16B16A16_SFLOAT, VK_FORMAT_R32G32B32A32_SFLOAT};

	VkPipelineColorBlendAttachmentState colorBlendAttachment = {};
	colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
	colorBlendAttachment.blendEnable = VK_TRUE;
	colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
	colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
	colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD;
	colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
	colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
	colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD;

	std::vector<VkPipelineColorBlendAttachmentState> colorBlendAttachments = { colorBlendAttachment, colorBlendAttachment,
		colorBlendAttachment, colorBlendAttachment, colorBlendAttachment, colorBlendAttachment };

	colorBlendAttachment.blendEnable = VK_FALSE;
	colorBlendAttachments[5] = colorBlendAttachment;
	

	configurator.setRenderingFormats(renderingColorFormats, VK_FORMAT_D32_SFLOAT, VK_FORMAT_UNDEFINED);
	configurator.setViewportState(nativeWidth, nativeHeight, 0.0f, 1.0f, 0.0f, 0.0f);
	configurator.setScissorState(nativeWidth, nativeHeight, 0.0f, 0.0f);
	configurator.setInputAssemblyState(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST);
	configurator.setBackfaceCulling(VK_CULL_MODE_BACK_BIT);
	configurator.setColorBlendingState(colorBlendAttachments);
	configurator.setDepthState(VK_TRUE, VK_TRUE, VK_COMPARE_OP_LESS_OR_EQUAL);
	

	configurator.addVertexAttribInputDescription(0, 0, VK_FORMAT_R32G32B32A32_SFLOAT, 0); //Position
	configurator.addVertexAttribInputDescription(1, 0, VK_FORMAT_R32G32B32A32_SFLOAT, 4 * sizeof(float)); //normal
	configurator.addVertexAttribInputDescription(2, 0, VK_FORMAT_R32G32B32A32_SFLOAT, 8 * sizeof(float)); //tangent
	configurator.addVertexAttribInputDescription(3, 0, VK_FORMAT_R32G32B32A32_SFLOAT, 12 * sizeof(float)); //texcoord
	configurator.addVertexInputInputBindingDescription(0, sizeof(Vertex), VK_VERTEX_INPUT_RATE_VERTEX);

	uint32_t numSamplers = 10;

	_gBufferShader = VGM::ShaderProgram(sources, _gBufferPipelineLayout, configurator, _vulkan._device, numSamplers);
}

void Raytracer::initRaytraceShader()
{
	VkDescriptorSetLayout layouts[] = { _globalLayout ,_raytracerLayout1, _raytracerLayout2, _lightLayout, _samplerLayout, _drawInstanceLayout, _textureLayout};

	std::vector<std::pair<std::string, VkShaderStageFlagBits>> sources = {
		{"C:\\Users\\Eric\\projects\\VulkanRaytracing\\shaders\\SPIRV\\raytraceIluminationRGEN.spv", VK_SHADER_STAGE_RAYGEN_BIT_KHR},
		{"C:\\Users\\Eric\\projects\\VulkanRaytracing\\shaders\\SPIRV\\raytraceIluminationRMISS.spv", VK_SHADER_STAGE_MISS_BIT_KHR},
		{"C:\\Users\\Eric\\projects\\VulkanRaytracing\\shaders\\SPIRV\\raytraceShadowRMISS.spv", VK_SHADER_STAGE_MISS_BIT_KHR},
		{"C:\\Users\\Eric\\projects\\VulkanRaytracing\\shaders\\SPIRV\\raytraceIluminationRCHIT.spv", VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR}
	};
	
	VkPipelineLayoutCreateInfo createInfo = {};
	createInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	createInfo.pNext = nullptr;
	createInfo.flags = 0;
	createInfo.pushConstantRangeCount = 0;
	createInfo.setLayoutCount = std::size(layouts);
	createInfo.pSetLayouts = layouts;

	vkCreatePipelineLayout(_vulkan._device, &createInfo, nullptr, &_raytracePipelineLayout);
	
	uint32_t numSamplers = 10;

	_raytraceShader = RaytracingShader(sources, _raytracePipelineLayout, _vulkan._device, _maxRecoursionDepth, numSamplers);
}

void Raytracer::initPostProcessingChain()
{
	VkDescriptorSetLayout layouts[] = { _postProcessLayout, _postProcessInputOutputLayout };

	VkPipelineLayoutCreateInfo createInfo = {};
	createInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	createInfo.pNext = nullptr;
	createInfo.flags = 0;
	createInfo.pushConstantRangeCount = 0;
	createInfo.setLayoutCount = std::size(layouts);
	createInfo.pSetLayouts = layouts;

	vkCreatePipelineLayout(_vulkan._device, &createInfo, nullptr, &_postProcessPipelineLayout);

	VGM::ComputeShaderProgram accumulate("C:\\Users\\Eric\\projects\\VulkanRaytracing\\shaders\\SPIRV\\accumulateCOMP.spv", VK_SHADER_STAGE_COMPUTE_BIT, _postProcessPipelineLayout, _vulkan._device);
	VGM::ComputeShaderProgram meanFilter("C:\\Users\\Eric\\projects\\VulkanRaytracing\\shaders\\SPIRV\\edgeAvoidingATroisFilterCOMP.spv", VK_SHADER_STAGE_COMPUTE_BIT, _postProcessPipelineLayout, _vulkan._device);
	
	_colorMapingShader = VGM::ComputeShaderProgram("C:\\Users\\Eric\\projects\\VulkanRaytracing\\shaders\\SPIRV\\colorMapCOMP.spv", VK_SHADER_STAGE_COMPUTE_BIT, _postProcessPipelineLayout, _vulkan._device);

	_postProcessingChain.push_back(accumulate);
	//_postProcessingChain.push_back(meanFilter);
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

	_postProcessCommandBuffers.resize(_concurrencyCount);
	for (auto& c : _postProcessCommandBuffers)
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

		vkCreateFence(_vulkan._device, &fenceCreateInfo, nullptr, &_frameSynchroStructs[i]._frameFence);
		vkCreateSemaphore(_vulkan._device, &semaphoreCreateInfo, nullptr, &_frameSynchroStructs[i]._offsceenRenderSemaphore);
		vkCreateSemaphore(_vulkan._device, &semaphoreCreateInfo, nullptr, &_frameSynchroStructs[i]._defferedRenderSemaphore);
		vkCreateSemaphore(_vulkan._device, &semaphoreCreateInfo, nullptr, &_frameSynchroStructs[i]._postProcessSemaphore);
		vkCreateSemaphore(_vulkan._device, &semaphoreCreateInfo, nullptr, &_frameSynchroStructs[i]._raytraceSemaphore);
		vkCreateSemaphore(_vulkan._device, &semaphoreCreateInfo, nullptr, &_frameSynchroStructs[i]._presentSemaphore);
	}
}

void Raytracer::initDataBuffers()
{
	_drawDataIsntanceBuffers.resize(_concurrencyCount);
	_cameraBuffers.resize(_concurrencyCount);
	_globalRenderDataBuffers.resize(_concurrencyCount);
	_drawIndirectCommandBuffer.resize(_concurrencyCount);

	_accelerationStructure.topLevelAccelStructures.resize(_concurrencyCount);
	_accelerationStructure.instanceBuffers.resize(_concurrencyCount);

	for(unsigned int i = 0; i<_concurrencyCount; i++)
	{
		_drawDataIsntanceBuffers[i] = VGM::Buffer(VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, _maxDrawCount * sizeof(DrawData), _vulkan._generalPurposeAllocator, _vulkan._device);
		_drawIndirectCommandBuffer[i] = VGM::Buffer(VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT, _maxDrawCount * sizeof(VkDrawIndexedIndirectCommand), _vulkan._generalPurposeAllocator, _vulkan._device);
		_cameraBuffers[i] = VGM::Buffer(VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, sizeof(CameraData), _vulkan._generalPurposeAllocator, _vulkan._device);
		_globalRenderDataBuffers[i] = VGM::Buffer(VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, sizeof(GlobalRenderData), _vulkan._generalPurposeAllocator, _vulkan._device);
		_accelerationStructure.instanceBuffers[i] = VGM::Buffer(VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT
			| VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR, sizeof(VkAccelerationStructureInstanceKHR) * _maxDrawCount, _vulkan._generalPurposeAllocator, _vulkan._device);
	}

	
}

void Raytracer::initLightBuffers()
{
	_sunLightBuffers.resize(_concurrencyCount);
	_pointLightBuffers.resize(_concurrencyCount);
	_spotLightBuffers.resize(_concurrencyCount);
	for(unsigned int i = 0; i<_concurrencyCount; i++)
	{
		_pointLightBuffers[i] = VGM::Buffer(VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, _maxPointLighst * sizeof(PointLight), _vulkan._generalPurposeAllocator, _vulkan._device);
		_sunLightBuffers[i] = VGM::Buffer(VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, _maxPointLighst * sizeof(SunLight), _vulkan._generalPurposeAllocator, _vulkan._device);
		_spotLightBuffers[i] = VGM::Buffer(VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, _maxPointLighst * sizeof(SpotLight), _vulkan._generalPurposeAllocator, _vulkan._device);
	}
}
