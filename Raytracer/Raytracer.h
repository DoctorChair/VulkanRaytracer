#pragma once
#include "SDL.h"
#include "VulkanContext.h"
#include "Buffer.h"
#include "Framebuffer.h"
#include "CommandBuffer.h"
#include "CommandBufferAllocator.h"
#include "DescriptorSet.h"
#include "DescriptorSetAllocator.h"
#include "Texture.h"
#include "Shader.h"
#include "PipelineBuilder.h"

#include "glm/glm.hpp"

#include "Vertex.h"

#include "BLAS.h"
#include "TLAS.h"
#include "RaytracingShader.h"
#include "ShaderBindingTable.h"

#include <unordered_map>

struct GBuffer
{
	VGM::Framebuffer framebuffer;

	VGM::Texture positionBuffer;
	VkImageView positionView;
	VGM::Texture normalBuffer;
	VkImageView normalView;
	VGM::Texture idBuffer;
	VkImageView idView;
	VGM::Texture albedoBuffer;
	VkImageView albedoView;
	VGM::Texture depthBuffer;
	VkImageView depthView;
};

struct DefferedBuffer
{
	VGM::Framebuffer framebuffer;

	VGM::Texture colorBuffer;
	VkImageView colorView;
};

struct RaytraceBuffer
{
	VGM::Texture colorBuffer;
	VkImageView colorView;
};

struct MeshBuffer
{
	VGM::Buffer vertices;
	VGM::Buffer indices;
};

struct AccelerationStructure
{
	std::vector<BLAS> bottomLevelAccelStructures;
	TLAS topLevelAccelStructure;
	VGM::Buffer instanceBuffer;
};

struct Material
{
	uint32_t albedoIndex;
	uint32_t metallicIndex;
	uint32_t normalIndex;
	uint32_t roughnessIndex;
};

struct Mesh
{
	uint32_t indexOffset;
	uint32_t vertexOffset;
	uint32_t indicesCount;
	uint32_t vertexCount;
	uint32_t blasIndex;
	Material material;
};

struct Instance
{
	uint32_t blasIndex;
	uint32_t meshIndex;
};

struct DrawData
{
	glm::mat4 modelMatrix;
	Material material;
	uint32_t ID;
};

struct Model
{
	std::vector<Mesh> meshes;
};

struct globalRenderData
{
	float fog;
};

struct CameraData
{
	glm::mat4 viewMatrix;
	glm::mat4 inverseviewMatrix;
	glm::mat4 projectionMatrix;
	glm::mat4 inverseProjectionMatrix;
	glm::vec3 cameraPosition;
};

struct FrameSynchro
{
	VkSemaphore _availabilitySemaphore;
	VkSemaphore _presentSemaphore;
	VkSemaphore _offsceenRenderSemaphore;
	VkSemaphore _defferedRenderSemaphore;
	VkSemaphore _raytraceSemaphore;
	VkSemaphore _compositingSemaphore;
	VkFence _offsrceenRenderFence;
	VkFence _defferedRenderFence;
	VkFence _raytraceFence;
	VkFence _compositingFence;
};

class Raytracer
{
public:
	Raytracer() = default;
	void init(SDL_Window* window);
	Mesh loadMesh(std::vector<Vertex>& vertices, std::vector<uint32_t>& indices);

	uint32_t loadTexture(std::vector<unsigned char> pixels, uint32_t width, uint32_t height, uint32_t nr_channels);
	void loadModel();
	void drawMesh(Mesh mesh, glm::mat4 transform, uint32_t objectID);
	void update();
	void destroy();

private:
	void initDescriptorSetAllocator();
	void initDescriptorSetLayouts();
	void initDescripotrSets();
	void initGBufferShader();
	void initDefferedShader();
	void initRaytraceShader();
	void initCompositingShader();
	void initCommandBuffers();
	void initSyncStructures();
	void initDataBuffers();

	void initMeshBuffer();
	void initgBuffers();
	void initDefferedBuffers();
	void initRaytraceBuffers();
	void initPresentFramebuffers();
	void initTextureArrays();
	void initShaderBindingTable();

	void loadDummyMeshInstance();
	void initTLAS();

	void updateGBufferDescriptorSets();
	void updateDefferedDescriptorSets();
	void updateRaytraceDescripotrSets();
	void updateCompositingDescriptorSets();
	
	void drawOffscreen();
	void renderDeffered();
	void traceRays();
	void composit();

	VkPhysicalDeviceRayTracingPipelinePropertiesKHR _raytracingProperties;

	VGM::VulkanContext _vulkan;
	uint32_t windowWidth;
	uint32_t windowHeight;

	uint32_t _concurrencyCount = 3;
	uint32_t _maxDrawCount = 1000;
	uint32_t _maxTextureCount = 1024;
	uint32_t _maxTriangleCount = 10000000;
	uint32_t _maxMipMapLevels = 1;
	uint64_t _timeout = 1000000000;

	std::vector<VGM::Texture> _textures;
	std::vector<VkImageView> _views;

	std::vector<Mesh> _loadedMeshes;
	std::vector<Instance> _activeInstances;

	MeshBuffer _meshBuffer;
	AccelerationStructure _accelerationStructure;

	VGM::ShaderProgram _gBufferShader;
	VkPipelineLayout _gBufferPipelineLayout;
	std::vector<GBuffer> _gBufferChain;

	VGM::ShaderProgram _defferedShader;
	VkPipelineLayout _defferedPipelineLayout;
	std::vector<DefferedBuffer> _defferdBufferChain;

	RaytracingShader _raytraceShader;
	VkPipelineLayout _raytracePipelineLayout;
	ShaderBindingTable _shaderBindingTable;
	std::vector<RaytraceBuffer> _raytraceBufferChain;
	
	VGM::ShaderProgram _compositingShader;
	VkPipelineLayout _compositingPipelineLayout;
	std::vector<VGM::Framebuffer> _presentFramebuffers;

	VGM::DescriptorSetAllocator _descriptorSetAllocator;

	uint32_t _currentFrameIndex = 0;
	uint32_t _currentSwapchainIndex = 0;

	VGM::CommandBufferAllocator _renderCommandBufferAllocator;
	std::vector<VGM::CommandBuffer> _offsceenRenderCommandBuffers;
	std::vector<VGM::CommandBuffer> _defferedRenderCommandBuffers;
	std::vector<VGM::CommandBuffer> _compositingCommandBuffers;

	VGM::CommandBufferAllocator _computeCommandBufferAllocator;
	std::vector<VGM::CommandBuffer> _raytraceRenderCommandBuffers;
	VGM::CommandBuffer _raytraceBuildCommandBuffer;

	VGM::CommandBufferAllocator _transferCommandBufferAllocator;
	VGM::CommandBuffer _transferCommandBuffer;

	std::vector<FrameSynchro> _frameSynchroStructs;

	std::vector<VGM::Buffer> _cameraBuffers;
	std::vector<VGM::Buffer> _globalRenderDataBuffers;
	std::vector<VGM::Buffer> _drawDataIsntanceBuffers;
	std::vector<VGM::Buffer> _drawIndirectCommandBuffer;

	std::vector<VkDrawIndexedIndirectCommand> _drawCommandTransferCache;
	std::vector<DrawData> _drawDataTransferCache;

	VGM::DescriptorSetAllocator _offsecreenDescriptorSetAllocator;
	VGM::DescriptorSetAllocator _textureDescriptorSetAllocator;
	VGM::DescriptorSetAllocator _defferedDescriptorSetAllocator;
	VGM::DescriptorSetAllocator _raytracerDescriptorSetAllocator;
	VGM::DescriptorSetAllocator _compositingDescriptorSetAllocator;

	VkDescriptorSetLayout _globalLayout;
	VkDescriptorSetLayout _gBufferLayout1;

	VkDescriptorSetLayout _gBufferLayout2;

	VkDescriptorSetLayout _textureLayout;

	VkDescriptorSetLayout _defferedLayout;

	VkDescriptorSetLayout _raytracerLayout1;
	VkDescriptorSetLayout _raytracerLayout2;

	VkDescriptorSetLayout _compositingLayout;

	std::vector<VkDescriptorSet> _globalDescriptorSets;

	std::vector<VkDescriptorSet> _gBuffer1DescripotrSets;
	std::vector<VkDescriptorSet> _gBuffer2DescriptorSets;

	std::vector<VkDescriptorSet> _textureDescriptorSets;

	std::vector<VkDescriptorSet> _defferedDescriptorSets;

	std::vector<VkDescriptorSet> _raytracer1DescriptorSets;
	std::vector<VkDescriptorSet> _raytracer2DescriptorSets;

	std::vector<VkDescriptorSet> _compositingDescriptorSets;

	Mesh _dummyMesh;
};